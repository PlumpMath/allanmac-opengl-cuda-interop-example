// -*- compile-command: "nvcc -m 32 -arch sm_30 -Xptxas=-v -cubin kernel.cu"; -*-

//
//
//

surface<void,cudaSurfaceType2D> surf;

//
//
//

union pxl_rgbx_24
{
  uint1       b32;

  struct {
    unsigned  r  : 8;
    unsigned  g  : 8;
    unsigned  b  : 8;
    unsigned  na : 8;
  };
};

//
//
//

__global__
void
pxl_kernel(const int width)
{
  // pixel coordinates
  const int idx = (blockDim.x * blockIdx.x) + threadIdx.x;
  const int x   = idx % width;
  const int y   = idx / width;

  // pixel color
  const unsigned int ramp = (unsigned int)(((float)x / (float)(width-1)) * 255.0f + 0.5f);
  const unsigned int bar  = (y / 32) & 3;

  union pxl_rgbx_24  rgbx;

  rgbx.r  = (bar == 0) || (bar == 1) ? ramp : 0;
  rgbx.g  = (bar == 0) || (bar == 2) ? ramp : 0;
  rgbx.b  = (bar == 0) || (bar == 3) ? ramp : 0;
  rgbx.na = 255;

  // cudaBoundaryModeZero squelches out-of-bound writes
  surf2Dwrite(rgbx.b32, // even simpler: (unsigned int)clock()
              surf,
              x*sizeof(rgbx),
              y,
              cudaBoundaryModeZero);
}

//
//
//

#define PXL_KERNEL_THREADS_PER_BLOCK  64

extern "C"
cudaError_t
pxl_kernel_launcher(cudaArray_const_t array,
                    const int         width,
                    const int         height,
                    cudaStream_t      stream)
{
  cudaError_t cuda_err = cudaBindSurfaceToArray(surf,array);

  if (cuda_err)
    return cuda_err;

  const int blocks = (width * height + PXL_KERNEL_THREADS_PER_BLOCK - 1) / PXL_KERNEL_THREADS_PER_BLOCK;

  pxl_kernel<<<blocks,PXL_KERNEL_THREADS_PER_BLOCK,0,stream>>>(width);

  return cudaSuccess;
}

//
//
//