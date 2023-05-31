#include "renderutil.h"
#include <stdio.h>
#include <stdlib.h>
#include "common.h"

char* scanfilecontents(const char* name) {
    FILE *fp;
    long lSize;
    char *buffer;

    fp = fopen ( name , "rb" );
    if( !fp ) perror(name),exit(1);

    fseek( fp , 0L , SEEK_END);
    lSize = ftell( fp );
    rewind( fp );

    /* allocate memory for entire content */
    buffer = (char*)calloc( 1, lSize+1 );
    if( !buffer ) fclose(fp),fputs("memory alloc fails",stderr),exit(1);

    /* copy the file into the buffer */
    if( 1!=fread( buffer , lSize, 1 , fp) )
      fclose(fp),free(buffer),fputs("entire read fails",stderr),exit(1);

    /* do your work here, buffer is a string contains the whole text */

    fclose(fp);

    return buffer;
}

void check_shader_compile_errors(unsigned int shader, char type) {
    GLint success;
	GLchar infoLog[1024];
	if(type != 'P') {
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if(!success) {
			glGetShaderInfoLog(shader, 1024, NULL, infoLog);
			printf("SHADER COMPILATION ERROR: TYPE%c\n", type);
			printf("%s\n", infoLog);
            printf("------------------------------------------------------\n");
		}
	} else {
		glGetProgramiv(shader, GL_LINK_STATUS, &success);
		if(!success) {
			glGetProgramInfoLog(shader, 1024, NULL, infoLog);
			printf("SHADER LINKING ERROR: TYPE%c\n", type);
			printf("%s\n", infoLog);
            printf("------------------------------------------------------\n");
		}
	}
}

unsigned int create_shader(const char* vs, const char* fs) {
    const char* vertex_code = scanfilecontents(vs);
    const char* fragment_code = scanfilecontents(fs);

    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertex_code, NULL);
    glCompileShader(vertex);
    check_shader_compile_errors(vertex, 'V');

    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragment_code, NULL);
    glCompileShader(fragment);
    check_shader_compile_errors(fragment, 'F');

    unsigned int ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);

    glLinkProgram(ID);
    check_shader_compile_errors(ID, 'P');

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return ID;
}

unsigned int create_cshader(const char* cs) {
    const char* compute_code = scanfilecontents(cs);

    // compute shader
    unsigned int compute = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute, 1, &compute_code, NULL);
    glCompileShader(compute);
    check_shader_compile_errors(compute, 'C');

    // shader Program
    unsigned int ID = glCreateProgram();
    glAttachShader(ID, compute);
    glLinkProgram(ID);
    check_shader_compile_errors(ID, 'P');
    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(compute);

    return ID;
}
