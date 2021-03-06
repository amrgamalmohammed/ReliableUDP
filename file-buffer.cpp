#include "file-buffer.h"

#include <string.h>

FileBufferedWriter::FileBufferedWriter(const char* filename, const uint32_t size)
    :   buf_writing_window(0, size), file_writing_window(0, size),
        buf_size(size), threashold(0.75 * size) {
    buf = new char[buf_size];
    acked = new bool[buf_size];
    memset(acked, 0, buf_size);
    of.open(filename);
}

Range<uint32_t> FileBufferedWriter::write(const char* data, const Range<uint32_t>& r) {
    soft_reset();
    return write_in_buf(data, r);
}

uint32_t FileBufferedWriter::adjust() {
    while(buf_writing_window.len() > 0 && acked[buf_writing_window.start()]) {
        buf_writing_window = buf_writing_window.move_start_by(1);
        file_writing_window = file_writing_window.move_start_by(1);
    }
    return file_writing_window.start();
}

/// Return: total number of bytes written to file
uint32_t FileBufferedWriter::close() {
    hard_reset();
    of.close();
    return tot_written;
}

FileBufferedWriter::~FileBufferedWriter() {
    delete acked;
    delete buf;
}

Range<uint32_t> FileBufferedWriter::write_in_buf(const char* data, const Range<uint32_t>& r) {
    Range<uint32_t> sect = file_writing_window.intersect(r);
    if (sect.len() > 0) {
        Range<uint32_t> data_window(sect.start() - r.start(), sect.len());
        Range<uint32_t> buf_window(sect.start() - tot_written, sect.len());;

        memcpy(buf + buf_window.start(), data + data_window.start(), sect.len());
        memset(acked + buf_window.start(), 1, sect.len());
    }
    return sect;
}

bool FileBufferedWriter::soft_reset() {
    if (buf_writing_window.start() > threashold) {
        hard_reset();
        return true;
    }
    return false;
}

void FileBufferedWriter::hard_reset() {
    of.write(buf + buf_writing_window.start(), buf_size - buf_writing_window.len());
    tot_written = file_writing_window.start();
    for (uint32_t i = 0; i < buf_writing_window.len(); ++i) {
        buf[i] = buf[buf_writing_window.start() + i];
    }
    for (uint32_t i = 0; i < buf_writing_window.len(); ++i) {
        acked[i] = acked[buf_writing_window.start() + i];
    }
    memset(acked + buf_writing_window.len(), 0, buf_size - buf_writing_window.len());
    buf_writing_window = Range<uint32_t>(0, buf_size);
    file_writing_window = Range<uint32_t>(tot_written, buf_size);
}
