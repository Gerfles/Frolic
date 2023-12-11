// // rasterText.cpp
// //
// //

// #include "rasterFont.h"
// #include <iostream>

// bool RasterFont::load(Graphics* g, std::string filename, int charSize)
// {
//   if (!fontImage.load(g, filename, charSize, charSize))
//     {
//       return false;
//     }

//   m_charSize = charSize;
//   // previously used this to determine charsize
//   //charSize = fontImage.getWidth() / numColumns;

//   // fontImage.setFrameSize(m_charSize, m_charSize);

//   return true;
// }


// void RasterFont::draw(Graphics* g, std::string text, int x, int y)
// {
//   // check that font image exists
//   if (!fontImage.isLoaded())
//     {
//       return;
//     }


//   for (unsigned int i = 0; i < text.length(); i++)
//     {
//       // render letter to screen using image frame
//       //std::cout << "text[" << i << "] = " << text[i] << std::endl;
//       fontImage.drawFrame(g, x + m_charSize * i , y, text[i] - START_CHAR);      
//     }
// }


// void RasterFont::free()
// {
//   fontImage.free();
// }

