#include "game.h"

void
GenerateFramebuffer(uint32_t* FBO, uint32_t* RBO, uint32_t* Texture, int Width,
                    int Height)
{
  // Screen framebuffer, renderbuffer and texture setup
  glGenFramebuffers(1, FBO);
  glBindFramebuffer(GL_FRAMEBUFFER, *FBO);

  glGenTextures(1, Texture);
  glBindTexture(GL_TEXTURE_2D, *Texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_INT,
               NULL);
  // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB,
  // GL_UNSIGNED_INT,  NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *Texture, 0);
  glGenRenderbuffers(1, RBO);
  glBindRenderbuffer(GL_RENDERBUFFER, *RBO);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCREEN_WIDTH, SCREEN_HEIGHT);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *RBO);
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    assert(0 && "error: incomplete framebuffer!\n");
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void
GenerateSSAOFrameBuffer(uint32_t* FBO, uint32_t* SSAOTextureID, int Width,
                        int Height)
{
  glGenFramebuffers(1, FBO);
  glBindFramebuffer(GL_FRAMEBUFFER, *FBO);

  // SSAO Texture
  {
    glGenTextures(1, SSAOTextureID);
    glBindTexture(GL_TEXTURE_2D, *SSAOTextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, Width, Height, 0, GL_RGB, GL_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *SSAOTextureID, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    assert(0 && "error: incomplete framebuffer!\n");
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void
GenerateFloatingPointFBO(uint32_t* FBO, uint32_t* ColorTextureID, uint32_t* DepthStencilRBO,
                         int Width, int Height)
{
  assert(FBO);
  assert(ColorTextureID);

  glGenFramebuffers(1, FBO);
  glBindFramebuffer(GL_FRAMEBUFFER, *FBO);

  glGenTextures(1, ColorTextureID);

  glBindTexture(GL_TEXTURE_2D, *ColorTextureID);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, Width, Height, 0, GL_RGB, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *ColorTextureID, 0);

  if(DepthStencilRBO)
  {
    glGenRenderbuffers(1, DepthStencilRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, *DepthStencilRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, Width, Height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              *DepthStencilRBO);
  }
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    assert(0 && "error: incomplete framebuffer!\n");
  }
}

void
GenerateGeometryDepthFrameBuffer(uint32_t* FBO, uint32_t* ViewSpacePositionTextureID,
                                 uint32_t* VelocityTextureID, uint32_t* NormalTextureID,
                                 uint32_t* DepthTextureID)
{
  glGenFramebuffers(1, FBO);
  glBindFramebuffer(GL_FRAMEBUFFER, *FBO);

  // View Space Position
  {
    glGenTextures(1, ViewSpacePositionTextureID);
    glBindTexture(GL_TEXTURE_2D, *ViewSpacePositionTextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB, GL_FLOAT,
                 NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           *ViewSpacePositionTextureID, 0);
  }
  // Velocity
  {
    glGenTextures(1, VelocityTextureID);
    glBindTexture(GL_TEXTURE_2D, *VelocityTextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RG, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, *VelocityTextureID,
                           0);
  }
  // Normal
  {
    glGenTextures(1, NormalTextureID);
    glBindTexture(GL_TEXTURE_2D, *NormalTextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB, GL_FLOAT,
                 NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, *NormalTextureID,
                           0);
  }

  uint32_t attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
  glDrawBuffers(ARRAY_SIZE(attachments), attachments);

  // Depth
  {
    glGenTextures(1, DepthTextureID);
    glBindTexture(GL_TEXTURE_2D, *DepthTextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, SCREEN_WIDTH, SCREEN_HEIGHT, 0,
                 GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
                           *DepthTextureID, 0);
  }
  glBindTexture(GL_TEXTURE_2D, 0);
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    assert(0 && "error: incomplete framebuffer!\n");
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void
GenerateDepthFramebuffer(uint32_t* FBO, uint32_t* Texture)
{
  glGenFramebuffers(1, FBO);

  glGenTextures(1, Texture);
  glBindTexture(GL_TEXTURE_2D, *Texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCREEN_WIDTH, SCREEN_HEIGHT, 0,
               GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  float BorderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, BorderColor);
  glBindTexture(GL_TEXTURE_2D, 0);

  glBindFramebuffer(GL_FRAMEBUFFER, *FBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, *Texture, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void
GenerateShadowFramebuffer(uint32_t* FBO, uint32_t* Texture)
{
  glGenFramebuffers(1, FBO);

  glGenTextures(1, Texture);
  glBindTexture(GL_TEXTURE_2D, *Texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
               GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  float BorderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, BorderColor);
  glBindTexture(GL_TEXTURE_2D, 0);

  glBindFramebuffer(GL_FRAMEBUFFER, *FBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, *Texture, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void
BindNextFramebuffer(uint32_t* FBOs, uint32_t* CurrentFramebuffer)
{
  *CurrentFramebuffer = (*CurrentFramebuffer + 1) % FRAMEBUFFER_MAX_COUNT;
  glBindFramebuffer(GL_FRAMEBUFFER, FBOs[*CurrentFramebuffer]);
}

void
BindTextureAndSetNext(uint32_t* Textures, uint32_t* CurrentTexture)
{
  glBindTexture(GL_TEXTURE_2D, Textures[*CurrentTexture]);
  *CurrentTexture = (*CurrentTexture + 1) % FRAMEBUFFER_MAX_COUNT;
}

void
DrawTextureToFramebuffer(uint32_t VAO)
{
  glBindVertexArray(VAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
}
