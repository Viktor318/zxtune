/**
 *
 * @file
 *
 * @brief Preferences dialog interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "apps/zxtune-qt/ui/preferences/mixing.h"

class QWidget;
class PlaybackSupport;

namespace UI
{
  void ShowPreferencesDialog(QWidget& parent, PlaybackSupport& playback, const PlaybackState& state);
  //! @brief Show preferences opened at the mixer page applicable to the current tune (AY mixer by default)
  void ShowMixerDialog(QWidget& parent, PlaybackSupport& playback, const PlaybackState& state);
}  // namespace UI
