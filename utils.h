#ifndef FPSUBMIT_UTILS_H_
#define FPSUBMIT_UTILS_H_

#include <QString>

inline QString extractExtension(const QString &fileName)
{
	int pos = fileName.lastIndexOf('.');
	if (pos == -1) {
		return "";
	}
    return fileName.mid(pos + 1).toUpper();
}

#endif
