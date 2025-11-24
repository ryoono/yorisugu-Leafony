// Auto-generated logistic regression parameters (with standardization folded in)
// Classes:
//   0 = Kinoko (Right)
//   1 = Kinoko (Left)
//   2 = Takenoko (Right)
//   3 = Takenoko (Left)

// Accuracy on test set (trained on X_std): 0.997403
// NOTE:
//   These weights expect 18-dimensional INPUT FEATURES
//   that are RAW analogRead() values (0–1023),
//   because the effect of StandardScaler has been folded into LOGI_W / LOGI_B.

#pragma once

#define LOGI_NUM_CLASSES  4
#define LOGI_NUM_FEATURES 18

// shape: 4 x 18
const float LOGI_W[4][18] = {
    {-0.001150f, -0.000048f, -0.000483f, -0.014574f, -0.006470f, 0.002959f, -0.007836f, -0.006714f, 0.023815f, 0.028519f, -0.009030f, -0.005501f, -0.009236f, 0.003254f, 0.035436f, 0.006570f, -0.007197f, 0.008009f},
    {-0.007194f, -0.016230f, -0.008811f, -0.009085f, 0.003488f, -0.003790f, -0.014362f, -0.006255f, 0.000436f, 0.029730f, 0.005795f, -0.004808f, 0.009666f, -0.012019f, -0.004684f, 0.014124f, 0.031659f, -0.014096f},
    {0.009177f, 0.004145f, -0.010558f, 0.011819f, 0.010470f, 0.002841f, 0.011832f, 0.005108f, -0.014920f, -0.029496f, 0.004978f, 0.003305f, 0.002114f, 0.010719f, -0.012804f, -0.013142f, -0.018977f, -0.000393f},
    {-0.000833f, 0.012134f, 0.019852f, 0.011840f, -0.007487f, -0.002011f, 0.010367f, 0.007861f, -0.009331f, -0.028752f, -0.001742f, 0.007005f, -0.002543f, -0.001954f, -0.017948f, -0.007553f, -0.005485f, 0.006480f}
};

// length: 4
const float LOGI_B[4] = { 4.626888f, 8.354758f, -7.063268f, -5.918378f };
