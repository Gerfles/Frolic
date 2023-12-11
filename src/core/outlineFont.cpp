// // outlineFont.cpp
// //
// //
// #include "outlineFont.h"

// OutlineFont::OutlineFont()
// {
//   font = NULL;
//   renderedText = NULL;
//   width = 0;
//   height = 0;
// }

// OutlineFont::~OutlineFont()
// {
// // //   clearText();
//    if (font != NULL)
//     {
//       TTF_CloseFont(font);
//       font = NULL; 
//     }
// }

// void OutlineFont::clearText()
// {

//   if (renderedText != NULL)
//     {
//       SDL_DestroyTexture(renderedText);
//       renderedText = NULL;
//     }
// }

// void OutlineFont::free()
// {

//   //    char *stylename=TTF_FontFaceStyleName(font);
//   // printf("The name of the face in the font is: %s\n", stylename);
 

  
//   clearText();
//   if (font != NULL)
//     {
      
      
//       if(!TTF_WasInit()) 
//         {
//           printf("TTF_Init: %s\n", TTF_GetError());
    
//         }
     
//       TTF_CloseFont(font);


//       font = NULL;

      
//     }
// }

// bool OutlineFont::load(std::string fileName, int size)
// {
//   font = TTF_OpenFont(fileName.c_str(), size);

//   if (!font)
//     {
//       SDL_Log("Outline Font class unable to load font: %s", fileName.c_str());
//       return false;
//     }

//   return true;
// }


// void OutlineFont::getFontImage(Graphics* gfx, Image* image, std::string text, Uint8 r
// 			       ,Uint8 g, Uint8 b, Uint32 renderFlag, SDL_Color bgColor)
// {
//   createFontImage(gfx, text, r, g, b, renderFlag, bgColor);

//   image->loadFromTexture(renderedText, width, height);
// }


// void OutlineFont::createFontImage(Graphics* gfx, std::string text, Uint8 r, Uint8 g
// 				  ,Uint8 b, Uint32 renderFlag, SDL_Color bgColor)
// {
//   // create color struct
//   //?? could allow for alpha in the future
//   SDL_Color color = {r, g, b, 255};
//   SDL_Surface* textSurface = NULL;
      
//   if (renderFlag == SOLID)
//     {
//       // TTF_RenderText_Solid - quick and dirty
//       textSurface = TTF_RenderText_Solid(font, text.c_str(), color);
//     }
//   else if (renderFlag == SHADED)
//     {
//       // TTF_RenderText_Shaded - slow and nice, but with a solid box
//       textSurface = TTF_RenderText_Shaded(font, text.c_str(), color, bgColor);
//     }
//   else if (renderFlag == BLENDED)
//     {
//       // TTF_RenderText_Solid - Slow, Slow, Slow but ultra nice over another image
//       textSurface = TTF_RenderText_Blended(font, text.c_str(), color);
//     }
      
//   if (textSurface == NULL)
//     {
//       printf("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
//       return;
//     }

//   // create texture from surface pixels
//   renderedText = SDL_CreateTextureFromSurface(gfx->getBackBuffer(), textSurface);
//   if( renderedText == NULL )
//     {
//       printf("Unable to create texture from rendered text!"); 
//       return;
//     }

//   // free unused surface that was needed to create the texture
//   width = textSurface->w;
//   height = textSurface->h;
//   SDL_FreeSurface(textSurface);
// }


// void OutlineFont::draw(Graphics* gfx, std::string text, int x, int y,
// 		       Uint8 r, Uint8 g, Uint8 b, Uint32 renderFlag, SDL_Color bgColor)
// {
//   // TODO: test that this function works properly and that there are no memory leaks
//   //?? provisional. had to place this here because having the free texture in
//   //the createFontImage was causing the images to be clobbered after they were
//   //given to a button through getFontImage function.
//   // free previous image texture
//   clearText();

//   // create the texture to be drawn
//   createFontImage(gfx, text, r, g, b, renderFlag, bgColor);
  
//   // set image destination
//   SDL_Rect destRect = {x, y, width, height}; //?? make sure this work since w and h = 0
  
//   // render to backbuffer and update
//   SDL_RenderCopy(gfx->getBackBuffer(), renderedText, NULL, &destRect);

//   return;  
// }


// // overloaded to NOT create the drawn texture, instead just use what has already been loaded
// void OutlineFont::draw(Graphics* gfx, int x, int y)
// {
//   // set image destination
//   SDL_Rect destRect = {x, y, width, height}; 
  
//   // render to backbuffer and update
//   if (renderedText)
//     {
//       SDL_RenderCopy(gfx->getBackBuffer(), renderedText, NULL, &destRect);
//     }
//   return;
// }
