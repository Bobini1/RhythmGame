#pragma once

#include <QMetaType>
#include <QtGlobal>
#include <QtQml/qqmlregistration.h>

class Lr2SkinElementDescriptorValue {
    Q_GADGET
    QML_VALUE_TYPE(lr2SkinElementDescriptor)
    Q_PROPERTY(bool valid MEMBER valid)
    Q_PROPERTY(int index MEMBER index)
    Q_PROPERTY(int type MEMBER type)
    Q_PROPERTY(qreal z MEMBER z)
    Q_PROPERTY(bool usesActiveOptions MEMBER usesActiveOptions)
    Q_PROPERTY(bool usesSkinTime MEMBER usesSkinTime)
    Q_PROPERTY(bool usesElementSkinTime MEMBER usesElementSkinTime)
    Q_PROPERTY(bool useDirectElementSkinClock MEMBER useDirectElementSkinClock)
    Q_PROPERTY(bool needsManualElementSkinTime MEMBER needsManualElementSkinTime)
    Q_PROPERTY(int elementSkinClockMode MEMBER elementSkinClockMode)
    Q_PROPERTY(bool sourceHasFrameAnimation MEMBER sourceHasFrameAnimation)
    Q_PROPERTY(bool sourceTreeHasFrameAnimation MEMBER sourceTreeHasFrameAnimation)
    Q_PROPERTY(bool sourceTreeUsesChartAsset MEMBER sourceTreeUsesChartAsset)
    Q_PROPERTY(int directChartAssetSourceType MEMBER directChartAssetSourceType)
    Q_PROPERTY(bool barDistributionGraphSourceHasFrameAnimation MEMBER barDistributionGraphSourceHasFrameAnimation)
    Q_PROPERTY(bool usesSelectHeldButtonTimer MEMBER usesSelectHeldButtonTimer)
    Q_PROPERTY(bool usesLiveDstClock MEMBER usesLiveDstClock)
    Q_PROPERTY(bool usesLiveSourceClock MEMBER usesLiveSourceClock)
    Q_PROPERTY(bool usesLiveSelectClock MEMBER usesLiveSelectClock)
    Q_PROPERTY(bool usesDynamicDstTimer MEMBER usesDynamicDstTimer)
    Q_PROPERTY(bool usesDynamicSrcTimer MEMBER usesDynamicSrcTimer)
    Q_PROPERTY(bool spriteUsesDirectSkinClock MEMBER spriteUsesDirectSkinClock)
    Q_PROPERTY(int spriteSkinClockMode MEMBER spriteSkinClockMode)
    Q_PROPERTY(int spriteSourceSkinClockMode MEMBER spriteSourceSkinClockMode)
    Q_PROPERTY(int spriteStateOverrideKind MEMBER spriteStateOverrideKind)
    Q_PROPERTY(bool usesSpriteStateOverride MEMBER usesSpriteStateOverride)
    Q_PROPERTY(bool usesSpriteForceHidden MEMBER usesSpriteForceHidden)
    Q_PROPERTY(bool usesButtonFrameOverride MEMBER usesButtonFrameOverride)
    Q_PROPERTY(bool sourceMouseCursor MEMBER sourceMouseCursor)
    Q_PROPERTY(bool dstOffsetsEnabled MEMBER dstOffsetsEnabled)
    Q_PROPERTY(int dstOffsetSide MEMBER dstOffsetSide)
    Q_PROPERTY(int scratchRotationSide MEMBER scratchRotationSide)
    Q_PROPERTY(int dstTimer MEMBER dstTimer)
    Q_PROPERTY(int srcTimer MEMBER srcTimer)
    Q_PROPERTY(bool selectScrollSlider MEMBER selectScrollSlider)
    Q_PROPERTY(bool genericSlider MEMBER genericSlider)
    Q_PROPERTY(bool gameplayProgressSlider MEMBER gameplayProgressSlider)
    Q_PROPERTY(bool gameplayLaneCoverSlider MEMBER gameplayLaneCoverSlider)
    Q_PROPERTY(bool numberRefSlider MEMBER numberRefSlider)
    Q_PROPERTY(int buttonId MEMBER buttonId)
    Q_PROPERTY(bool numberUsesFocusedSelectState MEMBER numberUsesFocusedSelectState)

public:
    bool valid = false;
    int index = -1;
    int type = -1;
    qreal z = 0.0;
    bool usesActiveOptions = false;
    bool usesSkinTime = false;
    bool usesElementSkinTime = false;
    bool useDirectElementSkinClock = false;
    bool needsManualElementSkinTime = false;
    int elementSkinClockMode = 0;
    bool sourceHasFrameAnimation = false;
    bool sourceTreeHasFrameAnimation = false;
    bool sourceTreeUsesChartAsset = false;
    int directChartAssetSourceType = 0;
    bool barDistributionGraphSourceHasFrameAnimation = false;
    bool usesSelectHeldButtonTimer = false;
    bool usesLiveDstClock = false;
    bool usesLiveSourceClock = false;
    bool usesLiveSelectClock = false;
    bool usesDynamicDstTimer = false;
    bool usesDynamicSrcTimer = false;
    bool spriteUsesDirectSkinClock = false;
    int spriteSkinClockMode = 0;
    int spriteSourceSkinClockMode = 0;
    int spriteStateOverrideKind = 0;
    bool usesSpriteStateOverride = false;
    bool usesSpriteForceHidden = false;
    bool usesButtonFrameOverride = false;
    bool sourceMouseCursor = false;
    bool dstOffsetsEnabled = false;
    int dstOffsetSide = 1;
    int scratchRotationSide = 0;
    int dstTimer = 0;
    int srcTimer = 0;
    bool selectScrollSlider = false;
    bool genericSlider = false;
    bool gameplayProgressSlider = false;
    bool gameplayLaneCoverSlider = false;
    bool numberRefSlider = false;
    int buttonId = 0;
    bool numberUsesFocusedSelectState = false;
};

Q_DECLARE_METATYPE(Lr2SkinElementDescriptorValue)
