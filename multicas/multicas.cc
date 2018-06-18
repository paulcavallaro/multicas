#include "multicas.h"

namespace multicas {

namespace {

// Built-in Functions:
//
// type __atomic_exchange_n(type *ptr, type val, int memorder);
//
// type __atomic_load_n(type *ptr, int memorder);
//
// bool __atomic_compare_exchange_n(type *ptr, type *expected, type desired,
//                                  bool weak, int success_memorder,
//                                  int failure_memorder);

int64_t CAS1(int64_t *addr, int64_t old_val, int64_t new_val) {
  __atomic_compare_exchange_n(addr, &old_val, new_val, /*weak=*/false,
                              /*success_memorder=*/__ATOMIC_SEQ_CST,
                              /*failure_memorder=*/__ATOMIC_SEQ_CST);
  // If not equal, __atomic_compare_exchange_n will write the value read
  // into expected, which is why the argument is a pointer.
  return old_val;
}

int64_t TransmuteWrite(RDCSSDescriptor *d) {
  return (reinterpret_cast<int64_t>(d) |  0x01);
}

RDCSSDescriptor* TransmuteRead(int64_t addr) {
  // Lop off bottom bit
  return reinterpret_cast<RDCSSDescriptor*>(addr & 0xfffffffffffffffe);
}

void Complete(RDCSSDescriptor *d) {
  const int64_t v = __atomic_load_n(d->control_addr, __ATOMIC_SEQ_CST);
  if (v == d->expected_control_val) {
    // Install d->new_data_val into d->data_addr iff d->data_addr contains d.
    CAS1(d->data_addr, TransmuteWrite(d), d->new_data_val);
  } else {
    // Install d->expected_data_val into d->data_addr iff d->data_addr does NOT
    // contain d.
    CAS1(d->data_addr, TransmuteWrite(d), d->expected_data_val);
  }
}

bool IsDescriptor(int64_t addr) {
  // Check to see if lower bit set, if so then it's a descriptor address
  if ((addr & 0xfffffffffffffffe) != 0) return true;
  return false;
}
}  // namespace;

int64_t RDCSS(RDCSSDescriptor *d) {
  int64_t r;
  do {
    r = CAS1(d->data_addr, d->expected_data_val, TransmuteWrite(d));
    if (IsDescriptor(r)) Complete(TransmuteRead(r));
  } while (IsDescriptor(r));
  if (r == d->expected_data_val) Complete(d);
  return r;
}

int64_t RDCSSRead(int64_t *addr) {
  int64_t r;
  do {
    r = __atomic_load_n(addr, __ATOMIC_SEQ_CST);
    if (IsDescriptor(r)) Complete(TransmuteRead(r));
  } while (IsDescriptor(r));
  return r;
}

}  // namespace multicas;
