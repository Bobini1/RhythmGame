#pragma once

#include <QString>
#include <QVariant>
#include <QList>
#include <QMap>
#include <QMetaType>

namespace gameplay_logic::lr2_skin {

struct Lr2Dst
{
    Q_GADGET
    Q_PROPERTY(int time MEMBER time)
    Q_PROPERTY(int x MEMBER x)
    Q_PROPERTY(int y MEMBER y)
    Q_PROPERTY(int w MEMBER w)
    Q_PROPERTY(int h MEMBER h)
    Q_PROPERTY(int acc MEMBER acc)
    Q_PROPERTY(int a MEMBER a)
    Q_PROPERTY(int r MEMBER r)
    Q_PROPERTY(int g MEMBER g)
    Q_PROPERTY(int b MEMBER b)
    Q_PROPERTY(int blend MEMBER blend)
    Q_PROPERTY(int filter MEMBER filter)
    Q_PROPERTY(int angle MEMBER angle)
    Q_PROPERTY(int center MEMBER center)
    Q_PROPERTY(int sortId MEMBER sortId)
    Q_PROPERTY(int loop MEMBER loop)
    Q_PROPERTY(int timer MEMBER timer)
    Q_PROPERTY(int op1 MEMBER op1)
    Q_PROPERTY(int op2 MEMBER op2)
    Q_PROPERTY(int op3 MEMBER op3)
    Q_PROPERTY(int op4 MEMBER op4)
  public:
    int time = 0;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int acc = 0;
    int a = 255;
    int r = 255;
    int g = 255;
    int b = 255;
    int blend = 0;
    int filter = 0;
    int angle = 0;
    int center = 0;
    int sortId = 0;
    int loop = 0;
    int timer = 0;
    int op1 = 0;
    int op2 = 0;
    int op3 = 0;
    int op4 = 0;
};

struct Lr2SrcImage
{
    Q_GADGET
    Q_PROPERTY(int gr MEMBER gr)
    Q_PROPERTY(int x MEMBER x)
    Q_PROPERTY(int y MEMBER y)
    Q_PROPERTY(int w MEMBER w)
    Q_PROPERTY(int h MEMBER h)
    Q_PROPERTY(int div_x MEMBER div_x)
    Q_PROPERTY(int div_y MEMBER div_y)
    Q_PROPERTY(int cycle MEMBER cycle)
    Q_PROPERTY(int timer MEMBER timer)
    Q_PROPERTY(int op1 MEMBER op1)
    Q_PROPERTY(int op2 MEMBER op2)
    Q_PROPERTY(int op3 MEMBER op3)
    Q_PROPERTY(int op4 MEMBER op4)
    Q_PROPERTY(int resultChartType MEMBER resultChartType)
    Q_PROPERTY(int resultChartIndex MEMBER resultChartIndex)
    Q_PROPERTY(bool button MEMBER button)
    Q_PROPERTY(int buttonId MEMBER buttonId)
    Q_PROPERTY(int buttonClick MEMBER buttonClick)
    Q_PROPERTY(int buttonPanel MEMBER buttonPanel)
    Q_PROPERTY(int buttonPlusOnly MEMBER buttonPlusOnly)
    Q_PROPERTY(bool onMouse MEMBER onMouse)
    Q_PROPERTY(int hoverPanel MEMBER hoverPanel)
    Q_PROPERTY(int hoverX MEMBER hoverX)
    Q_PROPERTY(int hoverY MEMBER hoverY)
    Q_PROPERTY(int hoverW MEMBER hoverW)
    Q_PROPERTY(int hoverH MEMBER hoverH)
    Q_PROPERTY(bool mouseCursor MEMBER mouseCursor)
    Q_PROPERTY(bool slider MEMBER slider)
    Q_PROPERTY(int sliderDirection MEMBER sliderDirection)
    Q_PROPERTY(int sliderRange MEMBER sliderRange)
    Q_PROPERTY(int sliderType MEMBER sliderType)
    Q_PROPERTY(int sliderDisabled MEMBER sliderDisabled)
    Q_PROPERTY(int specialType MEMBER specialType)
    Q_PROPERTY(int side MEMBER side)
    Q_PROPERTY(QString source MEMBER source)
  public:
    enum SpecialType
    {
        None = 0,
        StageFile = 1,
        SolidBlack = 2,
        BackBmp = 3,
        Banner = 4,
        SolidWhite = 5
    };

    int gr = 0;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int div_x = 1;
    int div_y = 1;
    int cycle = 0;
    int timer = 0;
    int op1 = 0;
    int op2 = 0;
    int op3 = 0;
    int op4 = 0;
    int resultChartType = 0;
    int resultChartIndex = 0;
    bool button = false;
    int buttonId = 0;
    int buttonClick = 0;
    int buttonPanel = 0;
    int buttonPlusOnly = 0;
    bool onMouse = false;
    int hoverPanel = 0;
    int hoverX = 0;
    int hoverY = 0;
    int hoverW = 0;
    int hoverH = 0;
    bool mouseCursor = false;
    bool slider = false;
    int sliderDirection = 0;
    int sliderRange = 0;
    int sliderType = 0;
    int sliderDisabled = 0;
    int specialType = None;
    int side = 0;
    QString source;
};

struct Lr2SrcNumber
{
    Q_GADGET
    Q_PROPERTY(int gr MEMBER gr)
    Q_PROPERTY(int x MEMBER x)
    Q_PROPERTY(int y MEMBER y)
    Q_PROPERTY(int w MEMBER w)
    Q_PROPERTY(int h MEMBER h)
    Q_PROPERTY(int div_x MEMBER div_x)
    Q_PROPERTY(int div_y MEMBER div_y)
    Q_PROPERTY(int cycle MEMBER cycle)
    Q_PROPERTY(int timer MEMBER timer)
    Q_PROPERTY(int num MEMBER num)
    Q_PROPERTY(int align MEMBER align)
    Q_PROPERTY(int keta MEMBER keta)
    Q_PROPERTY(bool nowCombo MEMBER nowCombo)
    Q_PROPERTY(int side MEMBER side)
    Q_PROPERTY(int judgementIndex MEMBER judgementIndex)
    Q_PROPERTY(QString source MEMBER source)
  public:
    int gr = 0;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int div_x = 1;
    int div_y = 1;
    int cycle = 0;
    int timer = 0;
    int num = 0;
    int align = 0;
    int keta = 0;
    bool nowCombo = false;
    int side = 0;
    int judgementIndex = -1;
    QString source;
};

struct Lr2SrcText
{
    Q_GADGET
    Q_PROPERTY(int font MEMBER font)
    Q_PROPERTY(int st MEMBER st)
    Q_PROPERTY(int align MEMBER align)
    Q_PROPERTY(int edit MEMBER edit)
    Q_PROPERTY(int panel MEMBER panel)
    Q_PROPERTY(QString fontPath MEMBER fontPath)
    Q_PROPERTY(QString fontFamily MEMBER fontFamily)
    Q_PROPERTY(int fontSize MEMBER fontSize)
    Q_PROPERTY(int fontThickness MEMBER fontThickness)
    Q_PROPERTY(int fontType MEMBER fontType)
    Q_PROPERTY(bool bitmapFont MEMBER bitmapFont)
    Q_PROPERTY(bool readme MEMBER readme)
    Q_PROPERTY(int readmeId MEMBER readmeId)
    Q_PROPERTY(int readmeLineSpacing MEMBER readmeLineSpacing)
  public:
    int font = 0;
    int st = 0;
    int align = 0;
    int edit = 0;
    int panel = 0;
    QString fontPath;
    QString fontFamily;
    int fontSize = 0;
    int fontThickness = 0;
    int fontType = 0;
    bool bitmapFont = false;
    bool readme = false;
    int readmeId = 0;
    int readmeLineSpacing = 18;
};

struct Lr2SrcBarImage
{
    Q_GADGET
    Q_PROPERTY(int kind MEMBER kind)
    Q_PROPERTY(int row MEMBER row)
    Q_PROPERTY(int variant MEMBER variant)
    Q_PROPERTY(QVariant source MEMBER source)
    Q_PROPERTY(QVariantList sources MEMBER sources)
  public:
    enum Kind
    {
        BodyOff = 0,
        BodyOn = 1,
        Flash = 2,
        Lamp = 3,
        MyLamp = 4,
        RivalLamp = 5,
        Rank = 6,
        Rival = 7
    };

    int kind = BodyOff;
    int row = -1;
    int variant = 0;
    QVariant source;
    QVariantList sources;
};

struct Lr2SrcBarText
{
    Q_GADGET
    Q_PROPERTY(int titleType MEMBER titleType)
    Q_PROPERTY(int font MEMBER font)
    Q_PROPERTY(int st MEMBER st)
    Q_PROPERTY(int align MEMBER align)
    Q_PROPERTY(int edit MEMBER edit)
    Q_PROPERTY(int panel MEMBER panel)
    Q_PROPERTY(QString fontPath MEMBER fontPath)
    Q_PROPERTY(QString fontFamily MEMBER fontFamily)
    Q_PROPERTY(int fontSize MEMBER fontSize)
    Q_PROPERTY(int fontThickness MEMBER fontThickness)
    Q_PROPERTY(int fontType MEMBER fontType)
    Q_PROPERTY(bool bitmapFont MEMBER bitmapFont)
  public:
    int titleType = 0;
    int font = 0;
    int st = 0;
    int align = 0;
    int edit = 0;
    int panel = 0;
    QString fontPath;
    QString fontFamily;
    int fontSize = 0;
    int fontThickness = 0;
    int fontType = 0;
    bool bitmapFont = false;
};

struct Lr2SrcBarNumber
{
    Q_GADGET
    Q_PROPERTY(int kind MEMBER kind)
    Q_PROPERTY(int variant MEMBER variant)
    Q_PROPERTY(QVariant source MEMBER source)
  public:
    enum Kind
    {
        Level = 0
    };

    int kind = Level;
    int variant = 0;
    QVariant source;
};

struct Lr2SrcBarGraph
{
    Q_GADGET
    Q_PROPERTY(int gr MEMBER gr)
    Q_PROPERTY(int x MEMBER x)
    Q_PROPERTY(int y MEMBER y)
    Q_PROPERTY(int w MEMBER w)
    Q_PROPERTY(int h MEMBER h)
    Q_PROPERTY(int div_x MEMBER div_x)
    Q_PROPERTY(int div_y MEMBER div_y)
    Q_PROPERTY(int cycle MEMBER cycle)
    Q_PROPERTY(int timer MEMBER timer)
    Q_PROPERTY(int graphType MEMBER graphType)
    Q_PROPERTY(int direction MEMBER direction)
    Q_PROPERTY(int specialType MEMBER specialType)
    Q_PROPERTY(QString source MEMBER source)
  public:
    int gr = 0;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int div_x = 1;
    int div_y = 1;
    int cycle = 0;
    int timer = 0;
    int graphType = 0;
    int direction = 0;
    int specialType = Lr2SrcImage::None;
    QString source;
};

struct Lr2Element
{
    Q_GADGET
    Q_PROPERTY(int type MEMBER type)
    Q_PROPERTY(QVariant src MEMBER src)
    Q_PROPERTY(QVariantList dsts MEMBER dsts)
  public:
    // 0=image, 1=number, 2=text, 3=bar image, 4=bar text,
    // 5=bar number, 6=bargraph, 7=BGA, 8=play notes, 9=groove gauge,
    // 10=result gauge/score chart
    int type = -1;
    QVariant src;
    QVariantList dsts;
};

struct Lr2SkinData
{
    QList<Lr2Element> elements;
    int skinWidth = 640;
    int skinHeight = 480;
    QVariantList activeOptions;
    QVariantList usedOptions;
    QVariantList barLampVariants;
    QVariantList barRows;
    QVariantList helpFiles;
    QString transColor = "#000000";
    bool hasTransColor = false;
    bool reloadBanner = false;
    int startInput = 0;
    int sceneTime = 0;
    int loadStart = 0;
    int loadEnd = 0;
    int playStart = 2000;
    int fadeOut = 0;
    int skip = 0;
    int barCenter = 0;
    int barAvailableStart = 0;
    int barAvailableEnd = -1;
    QVariantList noteSources;
    QVariantList mineSources;
    QVariantList lnStartSources;
    QVariantList lnEndSources;
    QVariantList lnBodySources;
    QVariantList autoNoteSources;
    QVariantList autoMineSources;
    QVariantList autoLnStartSources;
    QVariantList autoLnEndSources;
    QVariantList autoLnBodySources;
    QVariantList noteDsts;
    QVariantList lineSources;
    QVariantList lineDsts;
};

class Lr2SkinParser
{
  public:
    static QList<Lr2Element> parse(const QString& path,
                                   const QVariantMap& settingValues = {},
                                   const QVariantList& activeOptions = {});
    static Lr2SkinData parseData(const QString& path,
                                 const QVariantMap& settingValues = {},
                                 const QVariantList& activeOptions = {});
};

auto
resolvePath(const QString& path) -> QString;

} // namespace gameplay_logic::lr2_skin
