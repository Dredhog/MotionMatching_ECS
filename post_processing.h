// ------------POST-PROCESSING------------
// NOTE(rytis): Post-processing was done pretty sloppily, so it will most likely require some
// kind of rewrite/reintegration in the future.
//
// For now, though, it should be good enough?? Might still want to improve some parts (like
// the hard-coded screen quad vertices).
//
// Currently post-processing shaders only affect scene elements (entities, cubemap). GUI and
// debug drawings *should* be untouched.

void PerformPostProcessing(game_state* GameState)
{
	BEGIN_TIMED_BLOCK(PostProcessing);
	glDisable(GL_DEPTH_TEST);
	// NOTE(Lukas): HDR tonemapping and bloom use HDR buffers which are floating point unlike the
	// remaining post effect stack
	if(GameState->R.PPEffects & (POST_HDRTonemap | POST_Bloom) || GameState->R.RenderVolumetricScattering)
	{
		if(GameState->R.PPEffects & POST_Bloom)
		{
			// Outputting Bright regions to HDR texture
			{
				// Reading from HdrFBO index 0 and writing to index 1
				glBindFramebuffer(GL_FRAMEBUFFER, GameState->R.HdrFBOs[1]);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				GLuint ShaderBrighRegionFilterID =
					GameState->Resources.GetShader(GameState->R.PostBrightRegion);

				glUseProgram(ShaderBrighRegionFilterID);
				glUniform1f(glGetUniformLocation(ShaderBrighRegionFilterID, "u_BloomLuminanceThreshold"),
										GameState->R.BloomLuminanceThreshold);
				{
					int tex_index = 0;
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, GameState->R.HdrTextures[0]); // HDR scene
					glUniform1i(glGetUniformLocation(ShaderBrighRegionFilterID, "ScreenTex"), tex_index);
					DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);
				}

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			for(int i = 0; i < GameState->R.BloomBlurIterationCount; i++)
			{
				// Horizontal blur bright regions
				{
					// Reading from HdrFBO index 1 and writing to index 2
					glBindFramebuffer(GL_FRAMEBUFFER, GameState->R.HdrFBOs[2]);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

					GLuint ShaderPostBloomBlurID =
						GameState->Resources.GetShader(GameState->R.PostBloomBlur);
					glUseProgram(ShaderPostBloomBlurID);

		int tex_index = 0;
					glActiveTexture(GL_TEXTURE0 + tex_index);
					glBindTexture(GL_TEXTURE_2D, GameState->R.HdrTextures[1]); // HDR scene
					glUniform1i(glGetUniformLocation(ShaderPostBloomBlurID, "ScreenTex"), tex_index);
					glUniform1i(glGetUniformLocation(ShaderPostBloomBlurID, "u_Horizontal"), true);
					DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);

					glBindTexture(GL_TEXTURE_2D, 0);
				}
				// Vertical blur bright regions
				{
					// Reading from HdrFBO index 2 and writing to index 1
					glBindFramebuffer(GL_FRAMEBUFFER, GameState->R.HdrFBOs[1]);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

					GLuint ShaderPostBloomBlurID =
						GameState->Resources.GetShader(GameState->R.PostBloomBlur);
					glUseProgram(ShaderPostBloomBlurID);

		int tex_index = 0;
					glActiveTexture(GL_TEXTURE0 + tex_index);
					glBindTexture(GL_TEXTURE_2D, GameState->R.HdrTextures[2]);
					glUniform1i(glGetUniformLocation(ShaderPostBloomBlurID, "ScreenTex"), tex_index);
					glUniform1i(glGetUniformLocation(ShaderPostBloomBlurID, "u_Horizontal"), false);
					DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);

					glBindTexture(GL_TEXTURE_2D, 0);
				}
			}
		}

		// Applying bloom and tonemapping (This writes to the LDR CurrentFrameBuffer FBO)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, GameState->R.ScreenFBOs[GameState->R.CurrentFramebuffer]);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			GLuint ShaderBloomTonemapID = GameState->Resources.GetShader(GameState->R.PostBloomTonemap);
			glUseProgram(ShaderBloomTonemapID);

			{
				int tex_index = 0;
				glActiveTexture(GL_TEXTURE0 + tex_index);
				glBindTexture(GL_TEXTURE_2D, GameState->R.HdrTextures[0]); // HDR screen input
				glUniform1i(glGetUniformLocation(ShaderBloomTonemapID, "u_ScreenTex"), tex_index);
			}

			if(GameState->R.PPEffects & POST_Bloom)
			{
				{
					int tex_index = 1;
					glActiveTexture(GL_TEXTURE0 + tex_index);
					glBindTexture(GL_TEXTURE_2D, GameState->R.HdrTextures[1]); // HDR Bloom
					glUniform1i(glGetUniformLocation(ShaderBloomTonemapID, "u_BloomTex"), tex_index);
				}
				glUniform1i(glGetUniformLocation(ShaderBloomTonemapID, "u_ApplyBloom"), true);
			}
			else
			{
				glUniform1i(glGetUniformLocation(ShaderBloomTonemapID, "u_ApplyBloom"), false);
			}

			glUniform1i(glGetUniformLocation(ShaderBloomTonemapID, "u_ApplyVolumetricScattering"),
									GameState->R.RenderVolumetricScattering);
			if(GameState->R.RenderVolumetricScattering)
			{
				{
					int tex_index = 2;
					glActiveTexture(GL_TEXTURE0 + tex_index);
					glBindTexture(GL_TEXTURE_2D,
												GameState->R.LightScatterTextures[0]); // Volumetric Scattering
					glUniform1i(glGetUniformLocation(ShaderBloomTonemapID, "u_ScatteredLightMap"),
											tex_index);
				}
			}

			glUniform1i(glGetUniformLocation(ShaderBloomTonemapID, "u_ApplyTonemapping"),
									(bool)(GameState->R.PPEffects & POST_HDRTonemap));
			glUniform1f(glGetUniformLocation(ShaderBloomTonemapID, "u_Exposure"),
									GameState->R.ExposureHDR);

			DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
	else // Transitions bufferst to LDR when HDR Tonemapping is off
	{
		glBindFramebuffer(GL_FRAMEBUFFER, GameState->R.ScreenFBOs[GameState->R.CurrentFramebuffer]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		GLuint PostDefaultShaderID = GameState->Resources.GetShader(GameState->R.PostDefaultShader);
		glUseProgram(PostDefaultShaderID);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, GameState->R.HdrTextures[0]);

		DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	if(GameState->R.PPEffects)
	{
		if(GameState->R.PPEffects & POST_EdgeOutline)
		{
			GLuint PostEdgeOutlineShaderID =
				GameState->Resources.GetShader(GameState->R.PostEdgeOutline);
			glUseProgram(PostEdgeOutlineShaderID);
			glUniform1f(glGetUniformLocation(PostEdgeOutlineShaderID, "cameraFarClipPlane"), GameState->Camera.FarClipPlane);
			glUniform1f(glGetUniformLocation(PostEdgeOutlineShaderID, "OffsetX"), 1.0 / SCREEN_WIDTH);
			glUniform1f(glGetUniformLocation(PostEdgeOutlineShaderID, "OffsetY"), 1.0 / SCREEN_HEIGHT);

			glBindFramebuffer(GL_FRAMEBUFFER, GameState->R.EdgeOutlineFBO);
			{
				int TexIndex = 1;

				glActiveTexture(GL_TEXTURE0 + TexIndex);
				glBindTexture(GL_TEXTURE_2D, GameState->R.GBufferPositionTexID);
				glUniform1i(glGetUniformLocation(PostEdgeOutlineShaderID, "PositionTex"), TexIndex);
				glActiveTexture(GL_TEXTURE0);

				DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);
			}

			GLuint PostEdgeBlendShaderID = GameState->Resources.GetShader(GameState->R.PostEdgeBlend);
			glUseProgram(PostEdgeBlendShaderID);
			BindNextFramebuffer(GameState->R.ScreenFBOs, &GameState->R.CurrentFramebuffer);
			{
				int TexIndex = 1;

				glActiveTexture(GL_TEXTURE0 + TexIndex);
				glBindTexture(GL_TEXTURE_2D, GameState->R.EdgeOutlineTexture);
				glUniform1i(glGetUniformLocation(PostEdgeBlendShaderID, "EdgeTex"), TexIndex);
				glActiveTexture(GL_TEXTURE0);
			}
			{
				int TexIndex = 2;

				glActiveTexture(GL_TEXTURE0 + TexIndex);
				BindTextureAndSetNext(GameState->R.ScreenTextures, &GameState->R.CurrentTexture);
				glUniform1i(glGetUniformLocation(PostEdgeBlendShaderID, "ScreenTex"), TexIndex);
				glActiveTexture(GL_TEXTURE0);
			}

			DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);
		}

		// TODO(Lukas) Make FXAA more efficient by using fewer more spaced-out luminance samples and
		// a seperate precomputed luminance texture for input
		// TODO(Lukas) Expose FXAA sensitivity parameters to UI
		if(GameState->R.PPEffects & POST_FXAA)
		{
			BindNextFramebuffer(GameState->R.ScreenFBOs, &GameState->R.CurrentFramebuffer);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			GLuint ShaderFXAAID = GameState->Resources.GetShader(GameState->R.PostFXAA);

			glUseProgram(ShaderFXAAID);

			BindTextureAndSetNext(GameState->R.ScreenTextures, &GameState->R.CurrentTexture);
			DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		if(GameState->R.PPEffects & (POST_Blur | POST_DepthOfField))
		{
			if(GameState->R.PostBlurStdDev != GameState->R.PostBlurLastStdDev)
			{
				GameState->R.PostBlurLastStdDev = GameState->R.PostBlurStdDev;
				GenerateGaussianBlurKernel(GameState->R.PostBlurKernel, BLUR_KERNEL_SIZE,
																	 GameState->R.PostBlurLastStdDev);
			}

			int32_t UnblurredInputTextureIndex = GameState->R.CurrentTexture;

			GLuint PostBlurHShaderID = GameState->Resources.GetShader(GameState->R.PostBlurH);
			glUseProgram(PostBlurHShaderID);


			BindNextFramebuffer(GameState->R.ScreenFBOs, &GameState->R.CurrentFramebuffer);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glUniform1f(glGetUniformLocation(PostBlurHShaderID, "Offset"), 1.0f / SCREEN_WIDTH);
			glUniform1fv(glGetUniformLocation(PostBlurHShaderID, "Kernel"), BLUR_KERNEL_SIZE,
									 GameState->R.PostBlurKernel);

			BindTextureAndSetNext(GameState->R.ScreenTextures, &GameState->R.CurrentTexture);
			DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);


			BindNextFramebuffer(GameState->R.ScreenFBOs, &GameState->R.CurrentFramebuffer);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			GLuint PostBlurVShaderID = GameState->Resources.GetShader(GameState->R.PostBlurV);
			glUseProgram(PostBlurVShaderID);

			glUniform1f(glGetUniformLocation(PostBlurVShaderID, "Offset"), 1.0f / SCREEN_HEIGHT);
			glUniform1fv(glGetUniformLocation(PostBlurVShaderID, "Kernel"), BLUR_KERNEL_SIZE,
									 GameState->R.PostBlurKernel);

			BindTextureAndSetNext(GameState->R.ScreenTextures, &GameState->R.CurrentTexture);
			DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);

			if(GameState->R.PPEffects & POST_DepthOfField)
			{
				BindNextFramebuffer(GameState->R.ScreenFBOs, &GameState->R.CurrentFramebuffer);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				GLuint ShaderDepthOfFieldID =
					GameState->Resources.GetShader(GameState->R.PostDepthOfField);

				glUseProgram(ShaderDepthOfFieldID);
				{
					int tex_index = 1;
					glActiveTexture(GL_TEXTURE0 + tex_index);
					glBindTexture(GL_TEXTURE_2D, GameState->R.GBufferPositionTexID);
					glUniform1i(glGetUniformLocation(ShaderDepthOfFieldID, "u_PositionMap"), tex_index);
				}
				{
					int tex_index = 2;
					glActiveTexture(GL_TEXTURE0 + tex_index);
					glBindTexture(GL_TEXTURE_2D, GameState->R.ScreenTextures[UnblurredInputTextureIndex]);
					glUniform1i(glGetUniformLocation(ShaderDepthOfFieldID, "u_InputMap"), tex_index);
				}
				{
					int tex_index = 3;
					glActiveTexture(GL_TEXTURE0 + tex_index);
					BindTextureAndSetNext(GameState->R.ScreenTextures, &GameState->R.CurrentTexture);
					glUniform1i(glGetUniformLocation(ShaderDepthOfFieldID, "u_BlurredMap"), tex_index);
				}

				DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);
				glActiveTexture(GL_TEXTURE0);
			}
		}

		if(GameState->R.PPEffects & POST_MotionBlur)
		{
			BindNextFramebuffer(GameState->R.ScreenFBOs, &GameState->R.CurrentFramebuffer);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			GLuint ShaderMotionBlurID = GameState->Resources.GetShader(GameState->R.PostMotionBlur);

			glUseProgram(ShaderMotionBlurID);
			{
				int tex_index = 1;
				glActiveTexture(GL_TEXTURE0 + tex_index);
				glBindTexture(GL_TEXTURE_2D, GameState->R.GBufferVelocityTexID);
				glUniform1i(glGetUniformLocation(ShaderMotionBlurID, "u_VelocityMap"), tex_index);
			}
			{
				int tex_index = 2;
				glActiveTexture(GL_TEXTURE0 + tex_index);
				BindTextureAndSetNext(GameState->R.ScreenTextures, &GameState->R.CurrentTexture);
				glUniform1i(glGetUniformLocation(ShaderMotionBlurID, "u_InputMap"), tex_index);
			}

			DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);
			glActiveTexture(GL_TEXTURE0);
		}


		if(GameState->R.PPEffects & POST_Grayscale)
		{
			GLuint PostGrayscaleShaderID = GameState->Resources.GetShader(GameState->R.PostGrayscale);
			glUseProgram(PostGrayscaleShaderID);


			BindNextFramebuffer(GameState->R.ScreenFBOs, &GameState->R.CurrentFramebuffer);
			// glClear(GL_DEPTH_BUFFER_BIT);
			glActiveTexture(GL_TEXTURE0);
			BindTextureAndSetNext(GameState->R.ScreenTextures, &GameState->R.CurrentTexture);
			DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);
		}

		if(GameState->R.PPEffects & POST_NightVision)
		{
			glActiveTexture(GL_TEXTURE0);
			GLuint PostNightVisionShaderID =
				GameState->Resources.GetShader(GameState->R.PostNightVision);
			glUseProgram(PostNightVisionShaderID);

			BindNextFramebuffer(GameState->R.ScreenFBOs, &GameState->R.CurrentFramebuffer);
			BindTextureAndSetNext(GameState->R.ScreenTextures, &GameState->R.CurrentTexture);
			DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);
		}

		if(GameState->R.PPEffects & POST_SimpleFog)
		{
			GLuint PostSimpleFogShaderID = GameState->Resources.GetShader(GameState->R.PostSimpleFog);
			glUseProgram(PostSimpleFogShaderID);

			glUniform1f(glGetUniformLocation(PostSimpleFogShaderID, "cameraNearPlane"), GameState->Camera.NearClipPlane);
			glUniform1f(glGetUniformLocation(PostSimpleFogShaderID, "cameraFarPlane"), GameState->Camera.FarClipPlane);
			glUniform1f(glGetUniformLocation(PostSimpleFogShaderID, "density"), GameState->R.FogDensity);
			glUniform1f(glGetUniformLocation(PostSimpleFogShaderID, "gradient"), GameState->R.FogGradient);
			glUniform1f(glGetUniformLocation(PostSimpleFogShaderID, "fogColor"), GameState->R.FogColor);
			BindNextFramebuffer(GameState->R.ScreenFBOs, &GameState->R.CurrentFramebuffer);

			{
				int TexIndex = 1;

				glActiveTexture(GL_TEXTURE0 + TexIndex);
				glBindTexture(GL_TEXTURE_2D, GameState->R.GBufferPositionTexID);
				glUniform1i(glGetUniformLocation(PostSimpleFogShaderID, "PositionTex"), TexIndex);
				glActiveTexture(GL_TEXTURE0);
			}
			{
				int TexIndex = 2;

				glActiveTexture(GL_TEXTURE0 + TexIndex);
				BindTextureAndSetNext(GameState->R.ScreenTextures, &GameState->R.CurrentTexture);
				glUniform1i(glGetUniformLocation(PostSimpleFogShaderID, "ScreenTex"), TexIndex);
				glActiveTexture(GL_TEXTURE0);
			}

			DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);
		}

		if(GameState->R.PPEffects & POST_Noise)
		{
			GLuint PostNoiseShaderID =
				GameState->Resources.GetShader(GameState->R.PostNoise);
			glUseProgram(PostNoiseShaderID);
			glUniform1f(glGetUniformLocation(PostNoiseShaderID, "u_Time"), GameState->R.CumulativeTime);

			BindNextFramebuffer(GameState->R.ScreenFBOs, &GameState->R.CurrentFramebuffer);
			BindTextureAndSetNext(GameState->R.ScreenTextures, &GameState->R.CurrentTexture);
			DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);
		}

		if(GameState->R.PPEffects & POST_Test)
		{
			BindNextFramebuffer(GameState->R.ScreenFBOs, &GameState->R.CurrentFramebuffer);

			GLuint PostTestShaderID =
				GameState->Resources.GetShader(GameState->R.PostShaderTest);
			glUseProgram(PostTestShaderID);
			glUniform1f(glGetUniformLocation(PostTestShaderID, "u_Time"), GameState->R.CumulativeTime);

			{
				int TexIndex = 0;
				glActiveTexture(GL_TEXTURE0 + TexIndex);
				BindTextureAndSetNext(GameState->R.ScreenTextures, &GameState->R.CurrentTexture);
				glUniform1i(glGetUniformLocation(PostTestShaderID, "u_ScreenTex"), TexIndex);
			}
			{
				int TexIndex = 1;
				glActiveTexture(GL_TEXTURE0 + TexIndex);
				glBindTexture(GL_TEXTURE_2D, GameState->R.PostTestTextureID);
				glUniform1i(glGetUniformLocation(PostTestShaderID, "u_AuxiliaryTex"), TexIndex);
			}
			{
				int TexIndex = 2;
				glActiveTexture(GL_TEXTURE0 + TexIndex);
				glBindTexture(GL_TEXTURE_CUBE_MAP, GameState->R.Cubemap.CubemapTexture);
				glUniform1i(glGetUniformLocation(PostTestShaderID, "u_CubeTex"), TexIndex);
			}

			glActiveTexture(GL_TEXTURE0);

			DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);
			glActiveTexture(GL_TEXTURE0);
		}
	}

	if(GameState->R.DrawDepthBuffer)
	{
		GLuint RenderDepthMapShaderID = GameState->Resources.GetShader(GameState->R.RenderDepthMap);
		glUseProgram(RenderDepthMapShaderID);
		glUniform1f(glGetUniformLocation(RenderDepthMapShaderID, "cameraNearPlane"),
								GameState->Camera.NearClipPlane);
		glUniform1f(glGetUniformLocation(RenderDepthMapShaderID, "cameraFarPlane"),
								GameState->Camera.FarClipPlane);
		BindNextFramebuffer(GameState->R.ScreenFBOs, &GameState->R.CurrentFramebuffer);

		//Need this to keep current framebuffer and texture in sync
		GameState->R.CurrentTexture = (GameState->R.CurrentTexture + 1) % FRAMEBUFFER_MAX_COUNT;
		
		int TexIndex = 1;
		glActiveTexture(GL_TEXTURE0 + TexIndex);
		glBindTexture(GL_TEXTURE_2D, GameState->R.GBufferDepthTexID);
		glUniform1i(glGetUniformLocation(RenderDepthMapShaderID, "DepthMap"), TexIndex);
		DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);
		glActiveTexture(GL_TEXTURE0);
	}

	if(GameState->R.DrawShadowMap)
	{
		GLuint RenderShadowMapShaderID = GameState->Resources.GetShader(GameState->R.RenderShadowMap);
		glUseProgram(RenderShadowMapShaderID);
		BindNextFramebuffer(GameState->R.ScreenFBOs, &GameState->R.CurrentFramebuffer);

	GameState->R.CurrentTexture = (GameState->R.CurrentTexture + 1) % FRAMEBUFFER_MAX_COUNT;

		int TexIndex = 1;
		glActiveTexture(GL_TEXTURE0 + TexIndex);
		glBindTexture(GL_TEXTURE_2D, GameState->R.ShadowMapTextures[GameState->R.Sun.CurrentCascadeIndex]);
		glUniform1i(glGetUniformLocation(RenderShadowMapShaderID, "DepthMap"), TexIndex);
		DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);
		glActiveTexture(GL_TEXTURE0);
	}

	assert(GameState->R.CurrentTexture == GameState->R.CurrentFramebuffer);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, GameState->R.ScreenTextures[GameState->R.CurrentTexture]);
	DrawTextureToFramebuffer(GameState->R.ScreenQuadVAO);

	glEnable(GL_DEPTH_TEST);

	GameState->R.CurrentTexture     = 0;
	GameState->R.CurrentFramebuffer = 0;
	END_TIMED_BLOCK(PostProcessing);
}
