#if !defined(PCG_TABLEAU_F32_H)
#define PCG_TABLEAU_F32_H

#include <Image.h>
#include <Rgba32F.h>

namespace pcg
{

class Tableau
{
public:
    template <ScanLineMode S>
    static void fill (Image<Rgba32F, S> &img)
    {
        img.Alloc (PIXELS_W, PIXELS_H);
        size_t offset = 0;
        for (int h = 0; h < PIXELS_H; ++h) {
            Rgba32F * pix = img.GetScanlinePointer (h, TopDown);
            for (int w = 0; w < PIXELS_W; ++w) {
                pix[w].set(PIXELS_RGB[offset], 
                           PIXELS_RGB[offset+1],
                           PIXELS_RGB[offset+2]);
                offset += 3;
            }
        }
    }


private:
    static const float PIXELS_RGB[];
	static const int PIXELS_W = 512;
	static const int PIXELS_H = 512;
};

} // namespace pcg

#endif // PCG_TABLEAU_F32_H
