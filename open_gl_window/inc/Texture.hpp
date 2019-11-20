#include "glut.h"

class   Texture
{
public:
    enum    TextureStatus
    {
        TEXTURE_LOAD_SUCCESS = 0,
        TEXTURE_EMPTY = 1,
        TEXTURE_LOAD_FAILED = 2,
        TEXTURE_FREE_FAILED = 3,
        TEXTURE_FREE_SUCCESS = 4,
        TEXTURE_STATUS_TOTAL = 5,
    };

public:
    Texture();
    ~Texture(void);

public:
    TextureStatus&   loadTexture(const char* name, int width, int height);
    TextureStatus&   loadTextureFromPng(const char* name, int width, int height);
    TextureStatus&   freeTexture(void);

public:
    int             getWidth(void) const;
    int             getHeight(void) const;
    TextureStatus&  getStatus(void) const;
    GLuint          getTextureName(void) const;

private:
    TextureStatus   _status;
    GLuint          _textureId;
    int             _width;
    int             _height;
};