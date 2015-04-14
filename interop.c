
//
//
//

#include <stdlib.h>

//
//
//

#include "interop.h"

//
//
//

struct pxl_interop
{
  // w x h
  int                    width;
  int                    height;

  // GL buffers
  GLuint                 fb0;
  GLuint                 rb0;

  // CUDA resources
  cudaGraphicsResource_t cgr0;
};

//
//
//

struct pxl_interop*
pxl_interop_create(GLFWwindow* window)
{
  struct pxl_interop* const interop = (struct pxl_interop*)malloc(sizeof(struct pxl_interop));
  
  // init cuda graphics resource
  interop->cgr0 = NULL;

  // render buffer object w/a color buffer
  glGenRenderbuffers(1,&interop->rb0);

  // frame buffer object
  glGenFramebuffers(1,&interop->fb0);

  // return it
  return interop;
}


void
pxl_interop_destroy(struct pxl_interop* const interop)
{
  free(interop);
}

//
//
//

cudaError_t
pxl_interop_resize(struct pxl_interop* const interop, const int width, const int height)
{
  // save new size
  interop->width  = width;
  interop->height = height;

  //
  // RESIZE FBO'S COLOR BUFFER
  //
  
  // bind rbo
  glBindRenderbuffer   (GL_RENDERBUFFER,interop->rb0);
  // resize color buffer
  glRenderbufferStorage(GL_RENDERBUFFER,GL_RGBA8,width,height);

  // bind fbo to read
  glBindFramebuffer        (GL_FRAMEBUFFER,interop->fb0);
  // attach rb0 to the fb0
  glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER,interop->rb0);

  // unbind rbo
  glBindRenderbuffer(GL_RENDERBUFFER,0);

  // bind to default fb
  glBindFramebuffer(GL_FRAMEBUFFER,0);
  
  //
  // REGISTER RBO WITH CUDA
  //
  
  cudaError_t cuda_err;

  // unregister
  if (interop->cgr0 != NULL)
    cuda_err = cudaGraphicsUnregisterResource(interop->cgr0);

  // register image
  cuda_err = cudaGraphicsGLRegisterImage(&interop->cgr0,
                                         interop->rb0,
                                         GL_RENDERBUFFER,
                                         cudaGraphicsRegisterFlagsSurfaceLoadStore | cudaGraphicsRegisterFlagsWriteDiscard);

  // diddle some flags
  cuda_err = cudaGraphicsResourceSetMapFlags(interop->cgr0,cudaGraphicsMapFlagsWriteDiscard);

  return cuda_err;
}

void
pxl_interop_get_size(struct pxl_interop* const interop, int* const width, int* const height)
{
  *width  = interop->width;
  *height = interop->height;
}

//
//
//

cudaError_t
pxl_interop_map(struct pxl_interop* const interop, cudaArray_t* cuda_array, cudaStream_t stream)
{
  cudaError_t cuda_err;
  
  // map graphics resources
  cuda_err = cudaGraphicsMapResources(1,&interop->cgr0,stream);

  // get a CUDA Array
  cuda_err = cudaGraphicsSubResourceGetMappedArray(cuda_array,interop->cgr0,0,0);

  return cuda_err;
}

cudaError_t
pxl_interop_unmap(struct pxl_interop* const interop, cudaStream_t stream)
{
  cudaError_t cuda_err;
  
  cuda_err = cudaGraphicsUnmapResources(1,&interop->cgr0,stream);

  return cuda_err;
}
  
//
//
//

void
pxl_interop_clear(struct pxl_interop* const interop)
{
  /*
  const GLenum draw_buffer[] = { GL_COLOR_ATTACHMENT0 };
  const GLuint clear_color[] = { 255, 0, 0, 255 };
                       
  glNamedFramebufferDrawBuffers(interop->fb0,1,draw_buffer);
  glClearNamedFramebufferuiv(interop->fb0,GL_COLOR,0,clear_color);
  */

  const GLenum attachments[] = { GL_COLOR_ATTACHMENT0 };
  glInvalidateNamedFramebufferData(interop->fb0,1,attachments);
}

//
//
//

void
pxl_interop_blit(struct pxl_interop* const interop)
{
  /*
  glBlitFramebuffer(0,0,              interop->width,interop->height,
                    0,interop->height,interop->width,0,
                    GL_COLOR_BUFFER_BIT,
                    GL_NEAREST);
  */

  glBlitNamedFramebuffer(interop->fb0,0,
                         0,0,              interop->width,interop->height,
                         0,interop->height,interop->width,0,
                         GL_COLOR_BUFFER_BIT,
                         GL_NEAREST);
}

//
//
//