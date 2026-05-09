#include <stdio.h>

extern "C" {

struct Point {
    int x;
    int y;
};

struct ComplexData {
    int id;
    double value;
    const char* name;
};

__declspec(dllexport) void process_point(struct Point* p) {
    if (p) {
        printf("[NATIVE] Received Point: x=%d, y=%d\n", p->x, p->y);
        p->x += 100;
        p->y += 200;
    }
}

__declspec(dllexport) void process_complex(struct ComplexData* d) {
    if (d) {
        printf("[NATIVE] Received Complex: id=%d, value=%f, name=%s\n", d->id, d->value, (d->name ? d->name : "NULL"));
        d->id *= 2;
        d->value += 1.5;
    }
}

}
