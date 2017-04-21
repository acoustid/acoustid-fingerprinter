Acoustid Fingerprinter
======================

Dependencies
------------

 * Qt <http://qt.nokia.com/>
 * FFmpeg <http://www.ffmpeg.org/>
 * Chromaprint <http://wiki.acoustid.org/wiki/Chromaprint>

Installation
------------

    $ cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local .
    $ make
    $ sudo make install

### Debian dependencies

This can help to make compile process smoother :

    apt install libqt4-dev libtag1-dev libchromaprint-dev libavcodec-dev libavformat-dev
