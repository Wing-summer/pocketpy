#if !defined(SMALLMAP_T__HEADER) && !defined(SMALLMAP_T__SOURCE)
    #include "pocketpy/common/vector.h"

    #define SMALLMAP_T__HEADER
    #define SMALLMAP_T__SOURCE
    /* Input */
    #define K int
    #define V float
    #define TAG int_float
#endif

/* Optional Input */
#ifndef less
    #define less(a, b) ((a) < (b))
#endif

#ifndef equal
    #define equal(a, b) ((a) == (b))
#endif

/* Temprary macros */
#define partial_less(a, b)      less((a).key, (b))
#define CONCAT(A, B)            CONCAT_(A, B)
#define CONCAT_(A, B)           A##B

#define KV CONCAT(c11_smallmap_entry_, TAG)
#define SMALLMAP CONCAT(c11_smallmap_, TAG)
#define SMALLMAP_METHOD(name) CONCAT(SMALLMAP, CONCAT(__, name))

#ifdef SMALLMAP_T__HEADER
/* Declaration */
typedef struct {
    K key;
    V value;
} KV;

typedef c11_vector SMALLMAP;

void SMALLMAP_METHOD(ctor)(SMALLMAP* self);
void SMALLMAP_METHOD(dtor)(SMALLMAP* self);
void SMALLMAP_METHOD(set)(SMALLMAP* self, K key, V value);
V* SMALLMAP_METHOD(try_get)(const SMALLMAP* self, K key);
V SMALLMAP_METHOD(get)(const SMALLMAP* self, K key, V default_value);
bool SMALLMAP_METHOD(contains)(const SMALLMAP* self, K key);
bool SMALLMAP_METHOD(del)(SMALLMAP* self, K key);
void SMALLMAP_METHOD(clear)(SMALLMAP* self);

#endif

#ifdef SMALLMAP_T__SOURCE
/* Implementation */

void SMALLMAP_METHOD(ctor)(SMALLMAP* self) {
    c11_vector__ctor(self, sizeof(KV));
    c11_vector__reserve(self, 4);
}

void SMALLMAP_METHOD(dtor)(SMALLMAP* self) {
    c11_vector__dtor(self);
}

void SMALLMAP_METHOD(set)(SMALLMAP* self, K key, V value) {
    int index;
    c11__lower_bound(KV, self->data, self->count, key, partial_less, &index);
    KV* it = c11__at(KV, self, index);
    if(index != self->count && equal(it->key, key)) {
        it->value = value;
    } else {
        KV kv = {key, value};
        c11_vector__insert(KV, self, index, kv);
    }
}

V* SMALLMAP_METHOD(try_get)(const SMALLMAP* self, K key) {
    // use `bsearch` which is faster than `lower_bound`
    int low = 0;
    int high = self->count - 1;
    KV* a = self->data;
    while(low <= high){
        int mid = (low + high) / 2;
        if(equal(a[mid].key, key)){
            return &a[mid].value;
        } else if(less(a[mid].key, key)){
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return NULL;
}

V SMALLMAP_METHOD(get)(const SMALLMAP* self, K key, V default_value) {
    V* p = SMALLMAP_METHOD(try_get)(self, key);
    return p ? *p : default_value;
}

bool SMALLMAP_METHOD(contains)(const SMALLMAP* self, K key) {
    return SMALLMAP_METHOD(try_get)(self, key) != NULL;
}

bool SMALLMAP_METHOD(del)(SMALLMAP* self, K key) {
    int index;
    c11__lower_bound(KV, self->data, self->count, key, partial_less, &index);
    KV* it = c11__at(KV, self, index);
    if(index != self->count && equal(it->key, key)) {
        c11_vector__erase(KV, self, index);
        return true;
    }
    return false;
}

void SMALLMAP_METHOD(clear)(SMALLMAP* self) {
    c11_vector__clear(self);
}

#endif

/* Undefine all macros */
#undef KV
#undef SMALLMAP
#undef SMALLMAP_METHOD
#undef CONCAT
#undef CONCAT_

#undef K
#undef V
#undef TAG
#undef less
#undef partial_less
#undef equal