#pragma once

void VRRandomResetSeed(unsigned int seed = 0);
void VRRandomBackupSeed();
void VRRandomRestoreSeed();
float VRRandomFloat(float low, float high);
long VRRandomLong(long low, long high);
