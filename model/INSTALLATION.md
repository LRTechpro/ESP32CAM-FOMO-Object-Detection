# Edge Impulse Model Installation

The firmware expects the generated Edge Impulse Arduino library header:

```cpp
#include <HaroldLRWatkins-project-1_inferencing.h>
```

## Install the model library

1. Open the trained project in Edge Impulse Studio.
2. Select **Deployment**.
3. Choose **Arduino library**.
4. Build the quantized int8 package.
5. In Arduino IDE, select **Sketch → Include Library → Add .ZIP Library**.
6. Select the downloaded Edge Impulse ZIP file.
7. Restart Arduino IDE if the generated header is not immediately found.

The generated library is not duplicated in this repository because it is a build artifact tied to the corresponding Edge Impulse project export. The firmware, validation procedure, and required model header name are preserved here.
