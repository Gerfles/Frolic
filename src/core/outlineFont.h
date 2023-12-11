// // outlineFont.h
// //
//

// #ifndef OUTLINEFONT_H_
// #define OUTLINEFONT_H_

// #include <SDL2/SDL_ttf.h>
// #include <string>
// #include "graphics.h"
// #include "image.h"


// class OutlineFont
// {
// private:
//   TTF_Font* font;
//   SDL_Texture* renderedText;
//   // place to store the image dimensions for later use in dest. dimensions
//   int width, height;
 
// public:
//   // to determine what method SDL_ttf uses to render the font 
//   enum renderType
//     {
//      SOLID,
//      SHADED,
//      BLENDED,
//     };
//   OutlineFont();
//   ~OutlineFont();
//   bool load(std::string fileName, int size);

//   void createFontImage(Graphics* gfx, std::string text, Uint8 r, Uint8 g,
// 		       Uint8 b, Uint32 renderFlag = BLENDED,
//                        SDL_Color bgColor = {0x00,0x00,0x00,0xFF});

//   // allow a user to generate an image from the given font and string
//   // so that an image that remains stationary and doesn't change, will not need
//   // to be created ever time it needs to be drawn
//   void getFontImage(Graphics* gfx, Image* image, std::string text, Uint8 r
// 		    ,Uint8 g, Uint8 b, Uint32 renderFlag = BLENDED
// 		    ,SDL_Color bgColor = {0x00,0x00,0x00,0xFF});

//   void draw(Graphics* gfx, std::string text, int x, int y
// 	    , Uint8 r, Uint8 g, Uint8 b, Uint32 renderFlag = SOLID
// 	    , SDL_Color bgColor = {0x00,0x00,0x00,0xFF});
//   // overloaded to NOT create the drawn texture, instead just use what has
//   // already been loaded
//   void draw(Graphics* gfx, int x, int y);
//   // delete the string of text graphic in renderedText but don't close font
//   void clearText();
//   // delet renderedText and close font
//   void free();

//   // getters
//   int getWidth() { return width; }
//   int getHeight() { return height; }
// };

// #endif

