#include <string.h>
#include <stdlib.h>
#include "file_io.h"
#include "save_file.h"

int serialize_file_header(struct sf_stream *restrict stream,
			  const struct file_header *restrict header)
{
	sf_put_u32(stream, header->version);
	sf_put_u32(stream, header->save_num);
	sf_put_s(stream, header->ply_name);
	sf_put_s(stream, header->ply_location);
	sf_put_s(stream, header->game_time);
	sf_put_s(stream, header->ply_race_id);
	sf_put_u16(stream, header->ply_sex);
	sf_put_f32(stream, header->ply_current_xp);
	sf_put_f32(stream, header->ply_target_xp);
	sf_put_filetime(stream, header->filetime);
	sf_put_u32(stream, header->snapshot_width);
	sf_put_u32(stream, header->snapshot_height);

	if (header->version >= 12u)
		sf_put_u16(stream, header->compression_type);

	if (stream->status != SF_OK)
		return -1;

	return 0;
}

int deserialize_file_header(struct sf_stream *restrict stream,
			    struct file_header *restrict header)
{
	sf_get_u32(stream, &header->version);
	sf_get_u32(stream, &header->save_num);
	sf_get_ns(stream, header->ply_name, sizeof(header->ply_name));
	sf_get_u32(stream, &header->ply_level);
	sf_get_ns(stream, header->ply_location, sizeof(header->ply_location));
	sf_get_ns(stream, header->game_time, sizeof(header->game_time));
	sf_get_ns(stream, header->ply_race_id, sizeof(header->ply_race_id));
	sf_get_u16(stream, &header->ply_sex);
	sf_get_f32(stream, &header->ply_current_xp);
	sf_get_f32(stream, &header->ply_target_xp);
	sf_get_filetime(stream, &header->filetime);
	sf_get_u32(stream, &header->snapshot_width);
	sf_get_u32(stream, &header->snapshot_height);

	if (header->version >= 12u)
		sf_get_u16(stream, &header->compression_type);

	if (stream->status != SF_OK)
		return -1;

	return 0;
}
