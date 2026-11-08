/**
 *
 * @file
 *
 * @brief  Factory to create mixer
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "parameters/accessor.h"
#include "sound/matrix_mixer.h"

#include <vector>

namespace Sound
{
  void FillMixer(const Parameters::Accessor& params, ThreeChannelsMatrixMixer& mixer);
  void FillMixer(const Parameters::Accessor& params, FourChannelsMatrixMixer& mixer);

  Parameters::Accessor::Ptr CreateMixerNotificationParameters(Parameters::Accessor::Ptr delegate,
                                                              ThreeChannelsMatrixMixer::Ptr mixer);
  Parameters::Accessor::Ptr CreateMixerNotificationParameters(Parameters::Accessor::Ptr delegate,
                                                              FourChannelsMatrixMixer::Ptr mixer);
  //! @brief 3-channel mixers filled from the rows of (3 * mixers.size())-channel mixer parameters
  Parameters::Accessor::Ptr CreateMixerNotificationParameters(Parameters::Accessor::Ptr delegate,
                                                              std::vector<ThreeChannelsMatrixMixer::Ptr> mixers);
}  // namespace Sound
