#pragma once


void HLVR_LockGLMatrices(bool resetpushcount = true);
void HLVR_UnlockGLMatrices(bool resetpushcount=true);

bool InterceptOpenGLCalls();
