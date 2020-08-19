#include "net/url_encoder_decoder.h"

static const unsigned char hexchars[] = "0123456789ABCDEF";

int url_encode(char* out_buf_encoded, const size_t out_buf_encoded_size,
	const char* src_buf_plain, const size_t src_buf_plain_strlen)
{
	size_t input_index, output_index;
	char ch;

	if (src_buf_plain_strlen < 1 || out_buf_encoded_size < 2)
	{
		return -1;
	}

	for (input_index = 0, output_index = 0; input_index < src_buf_plain_strlen && output_index < out_buf_encoded_size - 1; input_index++)
	{
		ch = *(src_buf_plain + input_index);
		if ((ch >= 'A' && ch <= 'Z') ||
			(ch >= 'a' && ch <= 'z') ||
			(ch >= '0' && ch <= '9') ||
			ch == '.' || ch == '-' || ch == '*' || ch == '_')
		{
			out_buf_encoded[output_index++] = ch;
		}
		else if (ch == ' ')
		{
			out_buf_encoded[output_index++] = '+';
		}
		else
		{
			if (output_index + 3 < out_buf_encoded_size)
			{
				out_buf_encoded[output_index++] = '%';
				out_buf_encoded[output_index++] = hexchars[(unsigned char)ch >> 4];
				out_buf_encoded[output_index++] = hexchars[(unsigned char)ch & 0xF];
			}
			else
			{
				return -2;
			}
		}
	}

	if (input_index == 0)
	{
		return 0;
	}
	if (input_index == src_buf_plain_strlen)
	{
		out_buf_encoded[output_index] = '\0';
		return (int)output_index;
	}
	return -3;
}

int url_decode(char* out_buf_decoded, const size_t out_buf_decoded_size,
	const char* src_buf_plain, const size_t src_buf_plain_strlen, char** last_src_pos)
{
	size_t input_index, output_index;
	char source_char;
	char output_char;

	if (last_src_pos)
	{
		*last_src_pos = (char *)src_buf_plain;
	}
	if (src_buf_plain_strlen < 1 || out_buf_decoded_size < 2)
	{
		return -1;
	}

	for (input_index = 0, output_index = 0; input_index < src_buf_plain_strlen && output_index < out_buf_decoded_size - 1; output_index++)
	{
		source_char = *((char *)src_buf_plain + input_index);

		if (source_char == '+')
		{
			out_buf_decoded[output_index] = ' ';
			input_index += 1;
		}
		else if (source_char == '%')
		{
			if (input_index + 3 <= src_buf_plain_strlen)
			{
				source_char = *(src_buf_plain + input_index + 1);

				if (source_char >= 'A' && source_char <= 'F')
				{
					output_char = (source_char - 'A') + 10;
				}
				else if (source_char >= '0' && source_char <= '9')
				{
					output_char = source_char - '0';
				}
				else if (source_char >= 'a' && source_char <= 'f')
				{
					output_char = (source_char - 'a') + 10;
				}
				else
				{
					return -2;
				}

				output_char <<= 4;

				source_char = *(src_buf_plain + input_index + 2);
				if (source_char >= 'A' && source_char <= 'F')
				{
					output_char |= (source_char - 'A') + 10;
				}
				else if (source_char >= '0' && source_char <= '9')
				{
					output_char |= (source_char - '0');
				}
				else if (source_char >= 'a' && source_char <= 'f')
				{
					output_char |= (source_char - 'a') + 10;
				}
				else
				{
					return -2;
				}

				out_buf_decoded[output_index] = output_char;

				input_index += 3;
			}
			else
			{
				break;
			}
		}
		else if ((source_char >= 'A' && source_char <= 'Z') ||
			(source_char >= 'a' && source_char <= 'z') ||
			(source_char >= '0' && source_char <= '9') ||
			source_char == '.' || source_char == '-' || source_char == '*' || source_char == '_')
		{
			out_buf_decoded[output_index] = source_char;
			input_index += 1;
		}
		else
		{
			return -2;
		}
	}
	out_buf_decoded[output_index] = '\0';
	if (last_src_pos)
	{
		*last_src_pos = (char *)src_buf_plain + input_index;
	}
	return (int)output_index;
}
