#include "Texture.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <png.h>

Texture::Texture():
        _status(TEXTURE_EMPTY),
        _textureId(0),
        _width(0),
        _height(0){};

Texture::TextureStatus& Texture::loadTexture(const char* name, int width, int height)
{
    if (_status == TEXTURE_LOAD_SUCCESS)
    {
        return _status;
    }

    const int fd = open(name, O_RDONLY);
    if (fd == -1)
    {
        _status = TEXTURE_LOAD_FAILED;
        return _status;
    }
    const size_t BYTE_COUNT = width * height * 3;
    uint8_t* textureData = (uint8_t* )malloc(BYTE_COUNT);
    if (textureData == NULL)
    {
        _status = TEXTURE_LOAD_FAILED;
        return _status;
    }
    if (-1 == read(fd, textureData, BYTE_COUNT))
    {
        _status = TEXTURE_LOAD_FAILED;
        return _status;
    }
    close(fd);

    glGenTextures(1, &_textureId);
    glBindTexture(GL_TEXTURE_2D, _textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // the texture wraps over at the edges (repeat)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // build our texture mipmaps
    if (-1 == gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height, GL_RGB, GL_BITMAP, textureData))
    {
        _status = TEXTURE_LOAD_FAILED;
        return _status;
    }

    _status = TEXTURE_LOAD_SUCCESS;
    free(textureData);
    return _status;
};

Texture::TextureStatus& Texture::freeTexture()
{
    if (_status == TEXTURE_LOAD_SUCCESS)
    {
        glDeleteTextures(1, &_textureId);
        // free(_textureData);
        // _textureData = NULL;
        _status = TEXTURE_FREE_SUCCESS;
    }
    return _status;
};

Texture::~Texture()
{
    freeTexture();
};

GLuint  Texture::getTextureName(void) const
{
    return _textureId;
};

Texture::TextureStatus&  Texture::loadTextureFromPng(const char* filename, int width, int height)
{
    //header for testing if it is a png
    png_byte header[8];
    
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        _status = TEXTURE_LOAD_FAILED;
        return _status;
    }
 
    //read the header
    fread(header, 1, 8, fp);
 
    //test if png
    if (png_sig_cmp(header, 0, 8) != 0)
    {
        fclose(fp);
        _status = TEXTURE_LOAD_FAILED;
        return _status;
    }
    
    //create png struct
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL)
    {
        fclose(fp);
        _status = TEXTURE_LOAD_FAILED;
        return _status;
    }
    
    //create png info struct
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
        fclose(fp);
        _status = TEXTURE_LOAD_FAILED;
        return _status;
    }
 
    //create png info struct
    png_infop end_info = png_create_info_struct(png_ptr);
    if (end_info == NULL)
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
        fclose(fp);
        _status = TEXTURE_LOAD_FAILED;
        return _status;
    }
 
    //png error stuff, not sure libpng man suggests this.
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        _status = TEXTURE_LOAD_FAILED;
        return _status;
    }
 
    //init png reading
    png_init_io(png_ptr, fp);
 
    //let libpng know you already read the first 8 bytes
    png_set_sig_bytes(png_ptr, 8);
 
    // read all the info up to the image data
    png_read_info(png_ptr, info_ptr);
 
    //variables to pass to get info
    int bit_depth, color_type;
    png_uint_32 twidth, theight;
 
    // get info about png
    png_get_IHDR(png_ptr, info_ptr, &twidth, &theight, &bit_depth, &color_type, NULL, NULL, NULL);
 
    //update width and height based on png info
    width = twidth;
    height = theight;
 
    // Update the png info struct.
    png_read_update_info(png_ptr, info_ptr);
 
    // Row size in bytes.
    int rowbytes = png_get_rowbytes(png_ptr, info_ptr);
 
    // Allocate the image_data as a big block, to be given to opengl
    png_byte *image_data = new png_byte[rowbytes * height];
    if (image_data == NULL)
    {
        //clean up memory and close stuff
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        _status = TEXTURE_LOAD_FAILED;
        return _status;
    }
    
    //row_pointers is for pointing to image_data for reading the png with libpng
    png_bytep *row_pointers = new png_bytep[height];
    if (row_pointers == NULL)
    {
        //clean up memory and close stuff
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        delete[] image_data;
        fclose(fp);
        _status = TEXTURE_LOAD_FAILED;
        return _status;
    }
   
    // set the individual row_pointers to point at the correct offsets of image_data
    for (int i = 0; i < height; ++i)
    {
        row_pointers[height - 1 - i] = image_data + i * rowbytes;
    }
    
    //read the png into image_data through row_pointers
    png_read_image(png_ptr, row_pointers);
 
    //Now generate the OpenGL texture object
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D,0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*) image_data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 
    //clean up memory and close stuff
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    delete[] image_data;
    delete[] row_pointers;
    fclose(fp);
    
    _textureId = texture;
    _status = TEXTURE_LOAD_SUCCESS;
    return _status;
}