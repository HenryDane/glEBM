#ifndef _RENDER_UTIL_H
#define _RENDER_UTIL_H

char* scanfilecontents(const char* name);

void check_shader_compile_errors(unsigned int shader, char type);

unsigned int create_shader(const char* vs, const char* fs);

unsigned int create_cshader(const char* cs);

#endif
