#ifndef CEGSE_SAVE_FILE
#define CEGSE_SAVE_FILE

#include "file_io.h"
#include "types.h"

struct file_header {
	u32 version;		/* Creation Engine version */
	u32 save_num;
	char ply_name[64];	/* Player name */
	u32 ply_level;
	char ply_location[128];
	char game_time[48];	/* Playtime or in-game date */
	char ply_race_id[48];
	u16 ply_sex;
	f32 ply_current_xp;
	f32 ply_target_xp;
	FILETIME filetime;
	u32 snapshot_width;
	u32 snapshot_height;
	u16 compression_type;
};

/*
 * Serialize a file header to stream.
 *
 * On success, return nonnegative integer.
 * On failure, return -1.
 */
int serialize_file_header(struct sf_stream *restrict stream,
			  const struct file_header *restrict header);

/*
 * Deserialize a file header from stream. The header is expected
 * to be zero-initialized.
 *
 * On success, return nonnegative integer.
 * On failure, return -1.
 */
int deserialize_file_header(struct sf_stream *restrict stream,
			    struct file_header *restrict header);

#endif // CEGSE_SAVE_FILE
