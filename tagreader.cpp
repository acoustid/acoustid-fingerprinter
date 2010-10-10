#include <QFile>
#include <fileref.h>
#include <xiphcomment.h>
#include <apetag.h>
#include <vorbisfile.h>
#include <oggflacfile.h>
#include <speexfile.h>
#include <flacfile.h>
#include <mpcfile.h>
#include <wavpackfile.h>
#ifdef TAGLIB_WITH_ASF
#include <asffile.h>
#endif
#ifdef TAGLIB_WITH_MP4
#include <mp4file.h>
#endif
#include <mpegfile.h>
#include <id3v2tag.h>
#include <uniquefileidentifierframe.h>
#include "tagreader.h"

QMutex TagReader::m_mutex;

TagReader::TagReader(const QString &fileName)
    : m_fileName(fileName)
{
}

TagReader::~TagReader()
{
}

#define DISPATCH_TAGLIB_FILE(type, file) \
	{ \
		type *tmp = dynamic_cast<type *>(file); \
		if (tmp) { \
			return extractMBIDFromFile(tmp); \
		} \
	}

QString extractMBIDFromXiphComment(TagLib::Ogg::XiphComment *tag)
{
	const char *key = "MUSICBRAINZ_TRACKID"; 
	if (tag->fieldListMap().contains(key)) {
		return QString::fromUtf8(tag->fieldListMap()[key].front().toCString(true));
	}
	return QString();
}

QString extractMBIDFromAPETag(TagLib::APE::Tag *tag)
{
	const char *key = "MUSICBRAINZ_TRACKID";
	if (tag->itemListMap().contains(key)) {
		return QString::fromUtf8(tag->itemListMap()[key].toString().toCString(true));
	}
	return QString();
}

QString extractMBIDFromFile(TagLib::Ogg::Vorbis::File *file)
{
	return extractMBIDFromXiphComment(file->tag());
}

QString extractMBIDFromFile(TagLib::Ogg::FLAC::File *file)
{
	return extractMBIDFromXiphComment(file->tag());
}

QString extractMBIDFromFile(TagLib::Ogg::Speex::File *file)
{
	return extractMBIDFromXiphComment(file->tag());
}

QString extractMBIDFromFile(TagLib::FLAC::File *file)
{
	return extractMBIDFromXiphComment(file->xiphComment());
}

QString extractMBIDFromFile(TagLib::MPC::File *file)
{
	return extractMBIDFromAPETag(file->APETag());
}

QString extractMBIDFromFile(TagLib::WavPack::File *file)
{
	return extractMBIDFromAPETag(file->APETag());
}

#ifdef TAGLIB_WITH_ASF
QString extractMBIDFromFile(TagLib::ASF::File *file)
{
	const char *key = "MusicBrainz/Track Id";
	TagLib::ASF::Tag *tag = file->tag();
	if (tag->attributeListMap().contains(key)) {
		return QString::fromUtf8(tag->attributeListMap()[key].front().toString().toCString(true));
	}
	return QString();
}
#endif

#ifdef TAGLIB_WITH_MP4
QString extractMBIDFromFile(TagLib::MP4::File *file)
{
	const char *key = "----:com.apple.iTunes:MusicBrainz Track Id";
	TagLib::MP4::Tag *tag = file->tag();
	if (tag->itemListMap().contains(key)) {
		return QString::fromUtf8(tag->itemListMap()[key].toStringList().toString().toCString(true));
	}
	return QString();
}
#endif

QString extractMBIDFromFile(TagLib::MPEG::File *file)
{
	TagLib::ID3v2::Tag *tag = file->ID3v2Tag();
	TagLib::ID3v2::FrameList ufid = tag->frameListMap()["UFID"];
	if (!ufid.isEmpty()) {
		for (TagLib::ID3v2::FrameList::Iterator i = ufid.begin(); i != ufid.end(); i++) {
			TagLib::ID3v2::UniqueFileIdentifierFrame *frame = dynamic_cast<TagLib::ID3v2::UniqueFileIdentifierFrame *>(*i);
			if (frame && frame->owner() == "http://musicbrainz.org") {
				TagLib::ByteVector id = frame->identifier();
				return QString::fromAscii(id.data(), id.size());
			}
		}
	}
	return QString();
}

QString extractMusicBrainzTrackID(TagLib::File *file)
{
	DISPATCH_TAGLIB_FILE(TagLib::FLAC::File, file);
	DISPATCH_TAGLIB_FILE(TagLib::Ogg::Vorbis::File, file);
	DISPATCH_TAGLIB_FILE(TagLib::Ogg::FLAC::File, file);
	DISPATCH_TAGLIB_FILE(TagLib::Ogg::Speex::File, file);
	DISPATCH_TAGLIB_FILE(TagLib::MPC::File, file);
	DISPATCH_TAGLIB_FILE(TagLib::WavPack::File, file);
#ifdef TAGLIB_WITH_ASF
	DISPATCH_TAGLIB_FILE(TagLib::ASF::File, file);
#endif
#ifdef TAGLIB_WITH_MP4
	DISPATCH_TAGLIB_FILE(TagLib::MP4::File, file);
#endif
	DISPATCH_TAGLIB_FILE(TagLib::MPEG::File, file);
	return QString();
}

bool TagReader::read()
{
    // TagLib functions are not reentrant
    QMutexLocker locker(&m_mutex);

    QByteArray encodedFileName = QFile::encodeName(m_fileName);
	TagLib::FileRef file(encodedFileName.constData(), true);
	if (file.isNull()) {
		return false;
    }

	TagLib::Tag *tags = file.tag();	
	TagLib::AudioProperties *props = file.audioProperties();
	if (!tags || !props) {
        return false;
    }

	m_length = props->length();
    m_bitrate = props->bitrate();
	m_mbid = extractMusicBrainzTrackID(file.file());
	if (!m_length || m_mbid.size() != 36) {
		return false;
    }

    return true;
}
