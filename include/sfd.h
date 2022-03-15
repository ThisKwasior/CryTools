#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <mpeg.h>
#include <io_common.h>

void sfd_sofdec2_mpeg_packet(File_desc* fd, const uint8_t fd_size);