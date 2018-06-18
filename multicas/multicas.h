#include <cstdint>

namespace multicas {

struct RDCSSDescriptor {
  int64_t *control_addr;
  int64_t expected_control_val;
  int64_t *data_addr;
  int64_t expected_data_val;
  int64_t new_data_val;
};

int64_t RDCSS(RDCSSDescriptor *desc);
int64_t RDCSSRead(int64_t *addr);

}  // namespace multicas;
