#include <chrono>
#include <iostream>
#include <vector>

typedef struct {
    int obj_id;
    // minimum bounding rectangle 
    float left; 
    float right; 
    float top; 
    float bottom; 
} obj_t; 

typedef struct {
    int obj_id[2];
} pair_t;

std::vector<pair_t>* spatial_join_pages(
    std::vector<obj_t>& page_A, std::vector<obj_t>& page_B) {

    std::vector<pair_t>* result_intersect = new std::vector<pair_t>;

    for (obj_t obj_A : page_A) {
        for (obj_t obj_B: page_B) {

            // // point overlap is NOT regarded as overlap, only region overlap counts
            // bool overlap = 
            //     // horizontal overlap (no horizontal overlap within parentheses)
            //     !((obj_A.right <= obj_B.left) || (obj_B.right <= obj_A.left)) && 
            //     // vertical no overlap (no vertical overlap within parentheses)
            //     !((obj_A.bottom >= obj_B.top) || (obj_B.bottom >= obj_A.top));

            // point overlap is regarded as overlap, only region overlap counts
            bool overlap = 
                // horizontal overlap (no horizontal overlap within parentheses)
                !((obj_A.right < obj_B.left) || (obj_B.right < obj_A.left)) && 
                // vertical no overlap (no vertical overlap within parentheses)
                !((obj_A.bottom > obj_B.top) || (obj_B.bottom > obj_A.top));

            if (overlap) {
                pair_t result;
                result.obj_id[0] = obj_A.obj_id;
                result.obj_id[1] = obj_B.obj_id;
                result_intersect->push_back(result);
            }
        }
    }

    return result_intersect;
}

int main(int argc, char** argv) {

    int page_entry_num = 1000;
    std::cout << "Page entry num: " << page_entry_num << std::endl;

    std::vector<obj_t> page_A(page_entry_num);
    std::vector<obj_t> page_B(page_entry_num);


    std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
    std::vector<pair_t>* result_intersect = spatial_join_pages(page_A, page_B);
    std::chrono::system_clock::time_point end = std::chrono::system_clock::now();

    std::cout << "Intersect pair number: " << result_intersect->size() << std::endl;
    double durationUs = (std::chrono::duration_cast<std::chrono::microseconds>(end-start).count());
    std::cout << "Overall page per second = " << 1 / (durationUs / 1000.0 / 1000.0) << std::endl;
    std::cout << "Number of comparison (and potentially insertion) per second = " << 
        page_entry_num * page_entry_num / (durationUs / 1000.0 / 1000.0) << std::endl;

    delete result_intersect;

	return 0;
}