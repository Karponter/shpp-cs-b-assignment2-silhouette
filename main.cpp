#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include "gbufferedimage.h"

#define COL_BLACK 0            // numeric flag of black color marker
#define COL_WHITE 1            // numeric flag of white color marker
#define STATE_FREE 0           // numeric flag of free binarized image pixel
#define STATE_RESERVED 1       // numeric flag of reserved (added to watching queue) binarized image pixel
#define STATE_WATCHED 2        // numeric flag of watched binarized image pixel

using namespace std;

// class for managing image file and counting silohuettes in it
// constructable by path to image file with silhouettes on white background
class SilhoCounter {

private:

    // binarized pixel data struxture
    struct BinarizedPixel {
        int x;
        int y;
        char color;
        char state;
    };

    // defined local type of image binarization map
    typedef vector<vector<BinarizedPixel>> BinarizedMap;

    GBufferedImage * _gb_image;         // image buffer

    // color binarization filter
    // if any channel is more then 15 - consider pixel is black
    char binarizeColor(string rgb) {
        if (rgb[1] >= '1') return 1;
        if (rgb[3] >= '1') return 1;
        if (rgb[5] >= '1') return 1;
        return 0;
    }

public:

    // filename constructor.
    // f_name - path to image file with silhouettes
    SilhoCounter(string f_name) {
        _gb_image = new GBufferedImage();
        _gb_image->load(f_name);
    }

    // get all free nearby pixels from binarizes image
    vector<BinarizedPixel*> getNearbyPixels(BinarizedPixel *p) {
        int x = p->x, y = p->y;
        vector<BinarizedPixel*> retVal;
        if (x-1 >= 0 && _bin_map[x-1][y].state == STATE_FREE) retVal.push_back(&(_bin_map[x-1][y]));
        if (y-1 >= 0 && _bin_map[x][y-1].state == STATE_FREE) retVal.push_back(&(_bin_map[x][y-1]));
        if (x+1 < _gb_image->getWidth() && _bin_map[x+1][y].state == STATE_FREE) retVal.push_back(&(_bin_map[x+1][y]));
        if (y+1 < _gb_image->getHeight() && _bin_map[x][y+1].state == STATE_FREE) retVal.push_back(&(_bin_map[x][y+1]));
        return retVal;
    }

    // creates binary representation of buffered image
    BinarizedMap getBinarizedImage() {
        int H_lim = _gb_image->getHeight();
        int W_lim = _gb_image->getWidth();
        BinarizedMap retVal;
        for (int x = 0; x < W_lim; ++x) {
            vector<BinarizedPixel> pix_col;
            for (int y = 0; y < H_lim; ++y) {
                pix_col.push_back({
                    x, y,
                    binarizeColor(_gb_image->getRGBString(x, y)),
                    '\0'
                });
            }
            retVal.push_back(pix_col);
        }
        return retVal;
    }

    // count silhouettes in the buffered image
    int countSilhouettes() {

        // initializing unwatched binary map of buffered image
        BinarizedMap _bin_map = getBinarizedImage();
        // queue to store white pixels sequence
        queue<BinarizedPixel*> watch_queue;
        watch_queue.push(&_bin_map[0][0]);
        // vector stores size of each found silhouette
        vector<int> silho_counter;

        // outer level of width search
        // parsing all image, searches unwatched black pixels
        while (!watch_queue.empty()) {

            // dequeue first queue pixel
            BinarizedPixel *curr = watch_queue.front();
            watch_queue.pop();
            // looking for all neighbor pixels
            auto neighbor_pixels = getNearbyPixels(curr);
            for (auto it = neighbor_pixels.begin(); it < neighbor_pixels.end(); ++it) {
                // analizing only free-state neighbors, ignoring other
                if ((*it)->state == STATE_FREE) {
                    // adding white neighbors to queue
                    if ((*it)->color == COL_WHITE) {
                        (*it)->state = STATE_RESERVED;
                        watch_queue.push(*it);
                    } else {

                        int black_pix_counter = 0;
                        // queue to store black pixels sequence
                        // helps to realize width-search of silhouette borders
                        queue<BinarizedPixel*> black_queue;
                        black_queue.push(*it);
                        // inner level of width search
                        // cutting down all black pixels of the silhouette
                        // algorythm is mostly similar to the outer search loop
                        while (!black_queue.empty()) {
                            // one more black pixel is found
                            black_pix_counter++;
                            BinarizedPixel* curr_black = black_queue.front();
                            black_queue.pop();
                            auto neighbor_pixels_black = getNearbyPixels(curr_black);
                            for (int it_black = 0; it_black < neighbor_pixels_black.size(); ++it_black) {
                                if (neighbor_pixels_black[it_black]->state == STATE_FREE) {
                                    // adding white neighbors to queue
                                    // this hack allows to continue width search from the silhouette border
                                    if (neighbor_pixels_black[it_black]->color == COL_WHITE)
                                        watch_queue.push(neighbor_pixels_black[it_black]);
                                    // if neighbor is still black - adding it to appropriate queue to continue search process
                                    else black_queue.push(neighbor_pixels_black[it_black]);
                                    // marking analized neighbor as reserved in any case
                                    neighbor_pixels_black[it_black]->state = STATE_RESERVED;
                                }
                            }
                            // marking analized current black pixel as watched
                            curr_black->state = STATE_WATCHED;
                        }
                        // saving the number of pixels in current silhouette
                        silho_counter.push_back(black_pix_counter);
                        // turning back to outer level
                    }
                }
            }
            // marking analized current white pixel as watched
            curr->state = STATE_WATCHED;
        }

        // little hack that allows to ignore too small spots on the image
        int minimal_silho_size = 0;
        for (int i=0; i<silho_counter.size(); ++i)
            minimal_silho_size += silho_counter[i];
        // minimal_silho_size = avarage size of silhouette / 5;
        minimal_silho_size /= (silho_counter.size() * 5);

        // filtering found silhouettes
        int final_silho_number = 0;
        for (int i=0; i<silho_counter.size(); ++i)
            if (silho_counter[i] >= minimal_silho_size) final_silho_number++;

        return final_silho_number;
    }

    // just destructor
    ~SilhoCounter() {delete _gb_image;};

};

int main(int argc, char const *argv[]) {}

    auto SC = new SilhoCounter("test.jpg");
    cout << SC->countSilhouettes() << endl;
    delete SC;

    return 0;
}
