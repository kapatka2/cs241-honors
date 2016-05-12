/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall hello.c `pkg-config fuse --cflags --libs` -o hello
*/


/*
 Logging fuse filesystem by Rohan Kapatkar
*/

#define FUSE_USE_VERSION 26

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include <stdlib.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

FILE * f;


static int xmp_getattr(const char *path, struct stat *stbuf)
{
	
	fprintf(f, "getatrr: %s\n",path);
	int res = lstat(path, stbuf);
	if (res == -1){
		fprintf(f, "operation failed\n");
		return -errno;
	}
	fprintf(f,"operation succeeded\n");
	return 0;
}

static int xmp_access(const char *path, int mask)
{
	fprintf(f,"access: %s\n", path);
	int res = access(path, mask);
	if (res == -1){
		fprintf(f, "operation failed\n");
		return -errno;
	}

	fprintf(f, "operation succeeded\n");
	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
	fprintf(f,"readlink: %s\n", path);
	int res = readlink(path, buf, size - 1);
	if (res == -1){
		fprintf(f, "operation failed\n");
		return -errno;
	}
	buf[res] = '\0';
	fprintf(f, "operation succeeded\n");
	return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	fprintf(f,"readdir: %s\n",path);
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;

	dp = opendir(path);
	if (dp == NULL){
		fprintf(f, "operation failed\n");
		return -errno;
	}

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	fprintf(f, "operation succeeded\n");
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;
	fprintf(f, "mknod: %s, mode: %u\n", path, mode);
	if (S_ISREG(mode)) {
		res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(path, mode);
	else
		res = mknod(path, mode, rdev);
	if (res == -1){
		fprintf(f, "operation failed\n");
		return -errno;
	}
	fprintf(f,"operation succeeded\n");
	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
	fprintf(f, "mkdir: %s\n", path);
	int res = mkdir(path, mode);
	if (res == -1){
		fprintf(f,"operation failed\n");
		return -errno;
	}
	fprintf(f, "operation succeeded\n");
	return 0;
}

static int xmp_unlink(const char *path)
{

	fprintf(f, "unlink: %s\n", path);
	int res = unlink(path);
	if (res == -1){
		fprintf(f, "operation failed\n");
		return -errno;
	}
	fprintf(f, "operation succeeded\n");
	return 0;
}

static int xmp_rmdir(const char *path)
{
	fprintf(f, "unlink: %s\n",path);
	int res = rmdir(path);
	if (res == -1){
		fprintf(f, "operation failed\n");
		return -errno;
	}
	fprintf(f, "operation succeeded\n");
	return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
	fprintf(f,"symlink from: %s, to:%s\n",from ,to);
	int res = symlink(from, to);
	if (res == -1){
		fprintf(f, "operation failed\n");
		return -errno;
	}
	fprintf(f, "operation succeeded\n");

	return 0;
}

static int xmp_rename(const char *from, const char *to)
{
	fprintf(f, "rename from: %s, to: %s\n", from , to);
	int res = rename(from, to);
	if (res == -1){
		fprintf(f, "operation failed\n");
		return -errno;
	}
	fprintf(f, "operation succeeded\n");

	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	fprintf(f, "link from: %s, to: %s\n",from, to);
	int res = link(from, to);
	if (res == -1){
		fprintf(f, "operation failed\n");
		return -errno;
	}
	fprintf(f, "operation succeeded\n");
	return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
	fprintf(f, "chmod path: %s, mode: %u", path, mode);
	int res = chmod(path, mode);
	if (res == -1){
		fprintf(f, "operation failed\n");
		return -errno;	
	}
	fprintf(f, "operation succeeded\n");
	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
	fprintf(f, "chown: %s, uid: %u\n",path, uid);
	int res = lchown(path, uid, gid);
	if (res == -1){
		fprintf(f, "operation failed\n");
		return -errno;
	}
	fprintf(f, "operation succeeded\n");

	return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
	fprintf(f, "truncate path: %s, size: %lu\n", path, size);
	int res = truncate(path, size);
	if (res == -1){
		fprintf(f, "operation failed\n");
		return -errno;
	}
	fprintf(f, "operation succeeded\n");

	return 0;
}

#ifdef HAVE_UTIMENSAT
static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	fprintf(f, "utimes path: %s\n", path);
	int res = utimensat(0, path, ts, AT_SYMLINK_NOFOLLOW);
	if (res == -1){
		fprintf(f, "operation failed\n");
		return -errno;
	}
	fprintf(f, "operation succeeded\n");
	return 0;
}
#endif

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	fprintf(f, "open: %s, by %u\n", path, getuid());
	int res = open(path, fi->flags);
	if (res == -1){
		fprintf(f, "operation failed\n");
		return -errno;
	}
	fprintf(f, "operation succeeded\n");

	close(res);
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	fprintf(f,"read: %s, by %u\n",path, getuid());
	(void) fi;
	int fd = open(path, O_RDONLY);
	if (fd == -1){
		fprintf(f, "operation failed\n");
		return -errno;
	}

	int res = pread(fd, buf, size, offset);
	if (res == -1){
		fprintf(f, "operation failed\n");
		res = -errno;
	}
	close(fd);
	fprintf(f,"operation succeeded\n");
	return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	fprintf(f, "write: %s, by %u\n", path, getuid());
	(void) fi;
	int fd = open(path, O_WRONLY);
	if (fd == -1){
		fprintf(f, "operation failed\n");
		return -errno;
	}
	int res = pwrite(fd, buf, size, offset);
	if (res == -1){
		fprintf(f, "operation failed\n");
		res = -errno;
	}
	close(fd);
	fprintf(f, "operation succeeded\n");
	return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	fprintf(f, "statf path: %s\n", path);
	int res = statvfs(path, stbuf);
	if (res == -1){
		fprintf(f, "operation failed\n");
		return -errno;
	}
	fprintf(f,"operation succeeded\n");
	return 0;
}



#ifdef HAVE_POSIX_FALLOCATE
static int xmp_fallocate(const char *path, int mode,
			off_t offset, off_t length, struct fuse_file_info *fi)
{
	fprintf(f, "fallocate: %s\n", path);
	(void) fi;

	if (mode)
		return -EOPNOTSUPP;

	int fd = open(path, O_WRONLY);
	if (fd == -1)
		return -errno;

	int res = -posix_fallocate(fd, offset, length);

	close(fd);
	return res;
}
#endif

#ifdef HAVE_SETXATTR
static int xmp_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	fprintf(f,"setxattr: %s\n",path);
	int res = lsetxattr(path, name, value, size, flags);
	if (res == -1){
		fprintf(f, "operation failed\n");
		return -errno;
	}
	fprintf(f, "operation succeeded\n");
	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	fprintf(f, "getxattr: %s\n",path);
	int res = lgetxattr(path, name, value, size);
	if (res == -1){
		fprintf(f, "operation failed\n");
		return -errno;
	}
	fprintf(f, "operation succeeded\n");
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	fprintf(f, "listxattr: %s\n", path);
	int res = llistxattr(path, list, size);
	if (res == -1){
		fprintf(f, "operation failed\n");
		return -errno;
	}
	fprintf(f, "operation succeeded\n");
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	fprintf(f,"removexatrr: %s\n",path);
	int res = lremovexattr(path, name);
	if (res == -1){
		fprintf(f, "operation failed\n");
		return -errno;
	}
	fprintf(f, "operation succeeded\n");
	return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.access		= xmp_access,
	.readlink	= xmp_readlink,
	.readdir	= xmp_readdir,
	.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	.rename		= xmp_rename,
	.link		= xmp_link,
	.chmod		= xmp_chmod,
	.chown		= xmp_chown,
	.truncate	= xmp_truncate,
#ifdef HAVE_UTIMENSAT
	.utimens	= xmp_utimens,
#endif
	.open		= xmp_open,
	.read		= xmp_read,
	.write		= xmp_write,
	.statfs		= xmp_statfs,
#ifdef HAVE_POSIX_FALLOCATE
	.fallocate	= xmp_fallocate,
#endif
#ifdef HAVE_SETXATTR
	.setxattr	= xmp_setxattr,
	.getxattr	= xmp_getxattr,
	.listxattr	= xmp_listxattr,
	.removexattr	= xmp_removexattr,
#endif
};

int main(int argc, char *argv[])
{
        f  = fopen("logfile.txt","w+");
	if(f==NULL){
		perror("logfile");
		exit(1);
	}	
	umask(0);
	return fuse_main(argc, argv, &xmp_oper, NULL);
}
