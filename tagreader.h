/*
 * Chromaprint -- Audio fingerprinting toolkit
 * Copyright (C) 2010  Lukas Lalinsky <lalinsky@gmail.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#ifndef FPSUBMIT_TAGREADER_H_
#define FPSUBMIT_TAGREADER_H_

#include <QMutex>
#include <QString>

class TagReader
{
public:
    TagReader(const QString &fileName);
	~TagReader();

    bool read();

    QString mbid() const
    {
        return m_mbid;
    }

    int length() const
    {
        return m_length;
    }

    int bitrate() const
    {
        return m_bitrate;
    }

private:
    QString m_fileName;
    QString m_mbid;
    int m_bitrate;
    int m_length;
    static QMutex m_mutex;
};

#endif
