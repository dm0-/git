#define _OPEN_SYS_FILE_EXT 1
#include "cache.h"

int copy_fd(int ifd, int ofd)
{
#ifdef __MVS__
	struct stat ifs;
	if (fstat(ifd, &ifs) == 0) {
		attrib_t attr;
		memset(&attr, 0, sizeof(attr));
		attr.att_filetagchg = 1;
		attr.att_filetag = ifs.st_tag;
		__fchattr(ofd, &attr, sizeof(attr));
	}
#endif
	while (1) {
		char buffer[8192];
		ssize_t len = xread(ifd, buffer, sizeof(buffer));
		if (!len)
			break;
		if (len < 0) {
			return error("copy-fd: read returned %s",
				     strerror(errno));
		}
		if (write_in_full(ofd, buffer, len) < 0)
			return error("copy-fd: write returned %s",
				     strerror(errno));
	}
	return 0;
}

static int copy_times(const char *dst, const char *src)
{
	struct stat st;
	struct utimbuf times;
	if (stat(src, &st) < 0)
		return -1;
	times.actime = st.st_atime;
	times.modtime = st.st_mtime;
	if (utime(dst, &times) < 0)
		return -1;
	return 0;
}

int copy_file(const char *dst, const char *src, int mode)
{
	int fdi, fdo, status;

	mode = (mode & 0111) ? 0777 : 0666;
	if ((fdi = open(src, O_RDONLY)) < 0)
		return fdi;
	if ((fdo = open(dst, O_WRONLY | O_CREAT | O_EXCL, mode)) < 0) {
		close(fdi);
		return fdo;
	}
	status = copy_fd(fdi, fdo);
	close(fdi);
	if (close(fdo) != 0)
		return error("%s: close error: %s", dst, strerror(errno));

	if (!status && adjust_shared_perm(dst))
		return -1;

	return status;
}

int copy_file_with_time(const char *dst, const char *src, int mode)
{
	int status = copy_file(dst, src, mode);
	if (!status)
		return copy_times(dst, src);
	return status;
}
