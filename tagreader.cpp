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
    : m_fileName(fileName), m_trackNo(0), m_discNo(0), m_year(0)
{
}

TagReader::~TagReader()
{
}

#define TAGLIB_STRING_TO_QSTRING(a) QString::fromStdWString((a).toWString())

#define DISPATCH_TAGLIB_FILE(tr, type, file) \
	{ \
		type *tmp = dynamic_cast<type *>(file); \
		if (tmp) { \
			extractMetaFromFile(tr, tmp); \
			return; \
		} \
	}

void extractMetaFromXiphComment(TagReader *tr, TagLib::Ogg::XiphComment *tag)
{
	const char *key = "MUSICBRAINZ_TRACKID"; 
	if (tag->fieldListMap().contains(key)) {
		tr->m_mbid = TAGLIB_STRING_TO_QSTRING(tag->fieldListMap()[key].front());
	}
	key = "ALBUMARTIST"; 
	if (tag->fieldListMap().contains(key)) {
		tr->m_albumArtist = TAGLIB_STRING_TO_QSTRING(tag->fieldListMap()[key].front());
	}
	key = "DISCNUMBER"; 
	if (tag->fieldListMap().contains(key)) {
		tr->m_discNo = tag->fieldListMap()[key].front().toInt();
	}
}

void extractMetaFromAPETag(TagReader *tr, TagLib::APE::Tag *tag)
{
	const char *key = "MUSICBRAINZ_TRACKID";
	if (tag->itemListMap().contains(key)) {
		tr->m_mbid = TAGLIB_STRING_TO_QSTRING(tag->itemListMap()[key].toString());
	}
	key = "ALBUM ARTIST"; // Foobar
	if (tag->itemListMap().contains(key)) {
		tr->m_albumArtist = TAGLIB_STRING_TO_QSTRING(tag->itemListMap()[key].toString());
	}
	key = "ALBUMARTIST"; // Picard
	if (tag->itemListMap().contains(key)) {
		tr->m_albumArtist = TAGLIB_STRING_TO_QSTRING(tag->itemListMap()[key].toString());
	}
	key = "DISC"; 
	if (tag->itemListMap().contains(key)) {
		tr->m_discNo = tag->itemListMap()[key].toString().toInt();
	}
}

void extractMetaFromFile(TagReader *tr, TagLib::Ogg::Vorbis::File *file)
{
	extractMetaFromXiphComment(tr, file->tag());
}

void extractMetaFromFile(TagReader *tr, TagLib::Ogg::FLAC::File *file)
{
	extractMetaFromXiphComment(tr, file->tag());
}

void extractMetaFromFile(TagReader *tr, TagLib::Ogg::Speex::File *file)
{
	extractMetaFromXiphComment(tr, file->tag());
}

void extractMetaFromFile(TagReader *tr, TagLib::FLAC::File *file)
{
	extractMetaFromXiphComment(tr, file->xiphComment());
}

void extractMetaFromFile(TagReader *tr, TagLib::MPC::File *file)
{
	extractMetaFromAPETag(tr, file->APETag());
}

void extractMetaFromFile(TagReader *tr, TagLib::WavPack::File *file)
{
	extractMetaFromAPETag(tr, file->APETag());
}

#ifdef TAGLIB_WITH_ASF
void extractMetaFromFile(TagReader *tr, TagLib::ASF::File *file)
{
	const char *key = "MusicBrainz/Track Id";
	TagLib::ASF::Tag *tag = file->tag();
	if (tag->attributeListMap().contains(key)) {
		tr->m_mbid = TAGLIB_STRING_TO_QSTRING(tag->attributeListMap()[key].front().toString());
	}
	key = "WM/AlbumArtist";
	if (tag->attributeListMap().contains(key)) {
		tr->m_albumArtist = TAGLIB_STRING_TO_QSTRING(tag->attributeListMap()[key].front().toString());
	}
	key = "WM/PartOfSet";
	if (tag->attributeListMap().contains(key)) {
		tr->m_discNo = tag->attributeListMap()[key].front().toString().toInt();
	}
}
#endif

#ifdef TAGLIB_WITH_MP4
void extractMetaFromFile(TagReader *tr, TagLib::MP4::File *file)
{
	const char *key = "----:com.apple.iTunes:MusicBrainz Track Id";
	TagLib::MP4::Tag *tag = file->tag();
	if (tag->itemListMap().contains(key)) {
		tr->m_mbid = TAGLIB_STRING_TO_QSTRING(tag->itemListMap()[key].toStringList().toString());
	}
	key = "aART";
	if (tag->itemListMap().contains(key)) {
		tr->m_albumArtist = TAGLIB_STRING_TO_QSTRING(tag->itemListMap()[key].toStringList().toString());
	}
	key = "disk";
	if (tag->itemListMap().contains(key)) {
		tr->m_discNo = tag->itemListMap()[key].toIntPair().first;
	}
}
#endif

void extractMetaFromFile(TagReader *tr, TagLib::MPEG::File *file)
{
	TagLib::ID3v2::Tag *tag = file->ID3v2Tag();
	TagLib::ID3v2::FrameList ufid = tag->frameListMap()["UFID"];
	if (!ufid.isEmpty()) {
		for (TagLib::ID3v2::FrameList::Iterator i = ufid.begin(); i != ufid.end(); i++) {
			TagLib::ID3v2::UniqueFileIdentifierFrame *frame = dynamic_cast<TagLib::ID3v2::UniqueFileIdentifierFrame *>(*i);
			if (frame && frame->owner() == "http://musicbrainz.org") {
				TagLib::ByteVector id = frame->identifier();
				tr->m_mbid = QString::fromAscii(id.data(), id.size());
			}
		}
	}
	TagLib::ID3v2::FrameList tpe2 = tag->frameListMap()["TPE2"];
	if (!tpe2.isEmpty()) {
		tr->m_albumArtist = TAGLIB_STRING_TO_QSTRING(tpe2.front()->toString());
	}
	TagLib::ID3v2::FrameList tpos = tag->frameListMap()["TPOS"];
	if (!tpos.isEmpty()) {
		tr->m_discNo = tpos.front()->toString().toInt();
	}
}

void extractMeta(TagReader *tr, TagLib::File *file)
{
	DISPATCH_TAGLIB_FILE(tr, TagLib::FLAC::File, file);
	DISPATCH_TAGLIB_FILE(tr, TagLib::Ogg::Vorbis::File, file);
	DISPATCH_TAGLIB_FILE(tr, TagLib::Ogg::FLAC::File, file);
	DISPATCH_TAGLIB_FILE(tr, TagLib::Ogg::Speex::File, file);
	DISPATCH_TAGLIB_FILE(tr, TagLib::MPC::File, file);
	DISPATCH_TAGLIB_FILE(tr, TagLib::WavPack::File, file);
#ifdef TAGLIB_WITH_ASF
	DISPATCH_TAGLIB_FILE(tr, TagLib::ASF::File, file);
#endif
#ifdef TAGLIB_WITH_MP4
	DISPATCH_TAGLIB_FILE(tr, TagLib::MP4::File, file);
#endif
	DISPATCH_TAGLIB_FILE(tr, TagLib::MPEG::File, file);
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

	m_artist = TAGLIB_STRING_TO_QSTRING(tags->artist());
	m_album = TAGLIB_STRING_TO_QSTRING(tags->album());
	m_track = TAGLIB_STRING_TO_QSTRING(tags->title());
	m_trackNo = tags->track();
	m_year = tags->year();

	m_length = props->length();
    m_bitrate = props->bitrate();
	extractMeta(this, file.file());
	if (!m_length) {
		return false;
    }

    return true;
}
