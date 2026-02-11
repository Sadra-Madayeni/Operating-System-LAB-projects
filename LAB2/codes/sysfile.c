//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "string.h"
#include "file.h"
#include "fcntl.h"
#include "buf.h"
#include "x86.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

extern uint bmap(struct inode*, uint);

static int
argfd(int n, int *pfd, struct file **pf)
{
  int fd;
  struct file *f;

  if(argint(n, &fd) < 0)
    return -1;
  if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
    return -1;
  if(pfd)
    *pfd = fd;
  if(pf)
    *pf = f;
  return 0;
}

static int
fdalloc(struct file *f)
{
  int fd;
  struct proc *curproc = myproc();

  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd] == 0){
      curproc->ofile[fd] = f;
      return fd;
    }
  }
  return -1;
}

int
sys_dup(void)
{
  struct file *f;
  int fd;

  if(argfd(0, 0, &f) < 0)
    return -1;
  if((fd=fdalloc(f)) < 0)
    return -1;
  filedup(f);
  return fd;
}

int
sys_read(void)
{
  struct file *f;
  int n;
  char *p;

  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
    return -1;
  return fileread(f, p, n);
}

int
sys_write(void)
{
  struct file *f;
  int n;
  char *p;

  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
    return -1;
  return filewrite(f, p, n);
}

int
sys_close(void)
{
  int fd;
  struct file *f;

  if(argfd(0, &fd, &f) < 0)
    return -1;
  myproc()->ofile[fd] = 0;
  fileclose(f);
  return 0;
}

int
sys_fstat(void)
{
  struct file *f;
  struct stat *st;

  if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
    return -1;
  return filestat(f, st);
}

int
sys_link(void)
{
  char name[DIRSIZ], *new, *old;
  struct inode *dp, *ip;

  if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
    return -1;

  begin_op();
  if((ip = namei(old)) == 0){
    end_op();
    return -1;
  }

  ilock(ip);
  if(ip->type == T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  }

  ip->nlink++;
  iupdate(ip);
  iunlock(ip);

  if((dp = nameiparent(new, name)) == 0)
    goto bad;
  ilock(dp);
  if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
    iunlockput(dp);
    goto bad;
  }
  iunlockput(dp);
  iput(ip);

  end_op();

  return 0;

bad:
  ilock(ip);
  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);
  end_op();
  return -1;
}

static int
isdirempty(struct inode *dp)
{
  int off;
  struct dirent de;

  for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
    if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
      panic("isdirempty: readi");
    if(de.inum != 0)
      return 0;
  }
  return 1;
}

int
sys_unlink(void)
{
  struct inode *ip, *dp;
  struct dirent de;
  char name[DIRSIZ], *path;
  uint off;

  if(argstr(0, &path) < 0)
    return -1;

  begin_op();
  if((dp = nameiparent(path, name)) == 0){
    end_op();
    return -1;
  }

  ilock(dp);

  if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
    goto bad;

  if((ip = dirlookup(dp, name, &off)) == 0)
    goto bad;
  ilock(ip);

  if(ip->nlink < 1)
    panic("unlink: nlink < 1");
  if(ip->type == T_DIR && !isdirempty(ip)){
    iunlockput(ip);
    goto bad;
  }

  memset(&de, 0, sizeof(de));
  if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
    panic("unlink: writei");
  if(ip->type == T_DIR){
    dp->nlink--;
    iupdate(dp);
  }
  iunlockput(dp);

  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);

  end_op();

  return 0;

bad:
  iunlockput(dp);
  end_op();
  return -1;
}

static struct inode*
create(char *path, short type, short major, short minor)
{
  struct inode *ip, *dp;
  char name[DIRSIZ];

  if((dp = nameiparent(path, name)) == 0)
    return 0;
  ilock(dp);

  if((ip = dirlookup(dp, name, 0)) != 0){
    iunlockput(dp);
    ilock(ip);
    if(type == T_FILE && ip->type == T_FILE)
      return ip;
    iunlockput(ip);
    return 0;
  }

  if((ip = ialloc(dp->dev, type)) == 0)
    panic("create: ialloc");

  ilock(ip);
  ip->major = major;
  ip->minor = minor;
  ip->nlink = 1;
  iupdate(ip);

  if(type == T_DIR){  // Create . and .. entries.
    dp->nlink++;  // for ".."
    iupdate(dp);
    // No ip->nlink++ for ".": avoid cyclic ref count.
    if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
      panic("create dots");
  }

  if(dirlink(dp, name, ip->inum) < 0)
    panic("create: dirlink");

  iunlockput(dp);

  return ip;
}

int
sys_open(void)
{
  char *path;
  int fd, omode;
  struct file *f;
  struct inode *ip;

  if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
    return -1;

  begin_op();

  if(omode & O_CREATE){
    ip = create(path, T_FILE, 0, 0);
    if(ip == 0){
      end_op();
      return -1;
    }
  } else {
    if((ip = namei(path)) == 0){
      end_op();
      return -1;
    }
    ilock(ip);
    if(ip->type == T_DIR && omode != O_RDONLY){
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    if(f)
      fileclose(f);
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  end_op();

  f->type = FD_INODE;
  f->ip = ip;
  f->off = 0;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
  return fd;
}

int
sys_mkdir(void)
{
  char *path;
  struct inode *ip;

  begin_op();
  if(argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0){
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

int
sys_mknod(void)
{
  struct inode *ip;
  char *path;
  int major, minor;

  begin_op();
  if((argstr(0, &path)) < 0 ||
     argint(1, &major) < 0 ||
     argint(2, &minor) < 0 ||
     (ip = create(path, T_DEV, major, minor)) == 0){
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

int
sys_chdir(void)
{
  char *path;
  struct inode *ip;
  struct proc *curproc = myproc();
  
  begin_op();
  if(argstr(0, &path) < 0 || (ip = namei(path)) == 0){
    end_op();
    return -1;
  }
  ilock(ip);
  if(ip->type != T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  iput(curproc->cwd);
  end_op();
  curproc->cwd = ip;
  return 0;
}

int
sys_exec(void)
{
  char *path, *argv[MAXARG];
  int i;
  uint uargv, uarg;

  if(argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0){
    return -1;
  }
  memset(argv, 0, sizeof(argv));
  for(i=0;; i++){
    if(i >= NELEM(argv))
      return -1;
    if(fetchint(uargv+4*i, (int*)&uarg) < 0)
      return -1;
    if(uarg == 0){
      argv[i] = 0;
      break;
    }
    if(fetchstr(uarg, &argv[i]) < 0)
      return -1;
  }
  return exec(path, argv);
}

int
sys_pipe(void)
{
  int *fd;
  struct file *rf, *wf;
  int fd0, fd1;

  if(argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
    return -1;
  if(pipealloc(&rf, &wf) < 0)
    return -1;
  fd0 = -1;
  if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
    if(fd0 >= 0)
      myproc()->ofile[fd0] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  fd[0] = fd0;
  fd[1] = fd1;
  return 0;
}

int
sys_make_duplicate(void)
{
  char *src_path;
  char dest_path[MAXPATH];
  struct inode *ip_src, *ip_dest;
  uint off;
  int n, m;
  char buf[512];

  if(argstr(0, &src_path) < 0)
    return -1; 

  begin_op(); 

  if((ip_src = namei(src_path)) == 0){
    end_op(); 
    return -1; 
  }
  
  strncpy(dest_path, src_path, MAXPATH - 6); 
  dest_path[MAXPATH - 6] = '\0'; 
  strcat(dest_path, ".copy");

  if((ip_dest = create(dest_path, T_FILE, 0, 0)) == 0){
    iput(ip_src); 
    end_op(); 
    return -1;
  }

  ilock(ip_src);
  off = 0;
  while((n = readi(ip_src, buf, off, sizeof(buf))) > 0) {
    m = writei(ip_dest, buf, off, n);
    if(m != n){
      iunlock(ip_src);
      iunlock(ip_dest);
      iput(ip_src);
      iput(ip_dest);
      end_op(); 
      return -1;
    }
    off += n;
  }

  iunlock(ip_src);
  iunlock(ip_dest);
  iput(ip_src);
  iput(ip_dest);

  end_op(); 
  return 0;  
}

#include "fcntl.h" 

static char*
simplestr(const char *haystack, const char *needle)
{
    const char *h, *n;
    for (; *haystack; haystack++) {
        h = haystack;
        n = needle;
        while (*n && *h == *n) {
            h++;
            n++;
        }
        if (*n == '\0')  
            return (char*)haystack;
    }
    return 0;  
}


int
sys_grep_syscall(void)
{
    char *keyword, *filename, *user_buffer;
    int buffer_size;
    struct inode *ip = 0;
    char *buf = 0;  
    uint off, n, file_size;
    int result = -1;  

    if (argint(3, &buffer_size) < 0 || buffer_size <= 0 ||
        argstr(0, &keyword) < 0 || argstr(1, &filename) < 0 ||
        argptr(2, &user_buffer, buffer_size) < 0) {
        return -1;
    }

    if ((buf = kalloc()) == 0) {
        return -1;  
    }
    memset(buf, 0, PGSIZE); 

    begin_op();

    if ((ip = namei(filename)) == 0) {
        end_op();
        kfree(buf);
        return -1;
    }

    ilock(ip);
    if (ip->type != T_FILE || ip->size == 0 || ip->size > PGSIZE) {
        iunlockput(ip);
        end_op();
        kfree(buf);
        return -1;
    }

    file_size = ip->size;
    off = 0;
    while(off < file_size) {
        uint bytes_to_read = min(file_size - off, PGSIZE - off);
        if (bytes_to_read == 0) break;

        struct buf *bp;
        uint block_num = bmap(ip, off / BSIZE);
        if(block_num == 0) {
            iunlockput(ip);
            end_op();
            kfree(buf);
            return -1;
        }
        
        bp = bread(ip->dev, block_num);
        uint m = min(bytes_to_read, BSIZE - (off % BSIZE));
        memmove(buf + off, bp->data + (off % BSIZE), m);
        brelse(bp);
        off += m;
    }
    iunlockput(ip);
    end_op();
    int line_start = 0;
    int found = 0;
    for (n = 0; n <= file_size; n++) {
        if (n == file_size || buf[n] == '\n') {
            int line_len = n - line_start;
            if(line_len == 0) {
                line_start = n + 1;
                continue;
            }
            
            char original_char = buf[n];
            buf[n] = '\0'; 

            if (simplestr(buf + line_start, keyword)) {
                found = 1;
                if (line_len > buffer_size - 1) { 
                    line_len = buffer_size - 1;
                }
                if (copyout(myproc()->pgdir, (uint)user_buffer, buf + line_start, line_len) < 0) {
                    result = -1; 
                } else {
                    char nul = '\0';
                    copyout(myproc()->pgdir, (uint)user_buffer + line_len, &nul, 1);
                    result = line_len;  
                }
            }
            
            buf[n] = original_char; 
            line_start = n + 1; 

            if (found)
                break; 
        }
    }

    kfree(buf);
    return result;
}
