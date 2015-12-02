/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "icon.h"
#include "qtcassert.h"
#include "theme/theme.h"
#include "stylehelper.h"

#include <QApplication>
#include <QIcon>
#include <QImage>
#include <QMetaEnum>
#include <QPainter>
#include <QPaintEngine>
#include <QWidget>

namespace Utils {

static QPixmap maskToColorAndAlpha(const QPixmap &mask, const QColor &color)
{
    QImage result(mask.toImage().convertToFormat(QImage::Format_ARGB32));
    result.setDevicePixelRatio(mask.devicePixelRatio());
    QRgb *bitsStart = reinterpret_cast<QRgb*>(result.bits());
    const QRgb *bitsEnd = bitsStart + result.width() * result.height();
    const QRgb tint = color.rgb() & 0x00ffffff;
    const qreal alpha = color.alphaF();
    for (QRgb *pixel = bitsStart; pixel < bitsEnd; ++pixel) {
        QRgb pixelAlpha = ~(*pixel & 0xff) * alpha;
        *pixel = (pixelAlpha << 24) | tint;
    }
    return QPixmap::fromImage(result);
}

typedef QPair<QPixmap, QColor> MaskAndColor;
typedef QList<MaskAndColor> MasksAndColors;
static MasksAndColors masksAndColors(const Icon &icon, int dpr)
{
    MasksAndColors result;
    for (const IconMaskAndColor &i: icon) {
        const QString &fileName = i.first;
        const QColor color = creatorTheme()->color(i.second);
        const QString dprFileName = StyleHelper::availableImageResolutions(i.first).contains(dpr) ?
                    StyleHelper::imageFileWithResolution(fileName, dpr) : fileName;
        result.append(qMakePair(QPixmap(dprFileName), color));
    }
    return result;
}

static void smearPixmap(QPainter *painter, const QPixmap &pixmap, qreal radius)
{
    const qreal nagative = -radius - 0.01; // Workaround for QPainter rounding behavior
    const qreal positive = radius;
    painter->drawPixmap(QPointF(nagative, nagative), pixmap);
    painter->drawPixmap(QPointF(0, nagative), pixmap);
    painter->drawPixmap(QPointF(positive, nagative), pixmap);
    painter->drawPixmap(QPointF(positive, 0), pixmap);
    painter->drawPixmap(QPointF(positive, positive), pixmap);
    painter->drawPixmap(QPointF(0, positive), pixmap);
    painter->drawPixmap(QPointF(nagative, positive), pixmap);
    painter->drawPixmap(QPointF(nagative, 0), pixmap);
}

static QPixmap combinedMask(const MasksAndColors &masks)
{
    if (masks.count() == 1)
        return masks.first().first;

    QPixmap result(masks.first().first);
    QPainter p(&result);
    p.setCompositionMode(QPainter::CompositionMode_Darken);
    auto maskImage = masks.constBegin();
    maskImage++;
    for (;maskImage != masks.constEnd(); ++maskImage) {
        p.save();
        p.setOpacity(0.4);
        p.setCompositionMode(QPainter::CompositionMode_Lighten);
        smearPixmap(&p, maskToColorAndAlpha((*maskImage).first, Qt::white), 0.5);
        p.restore();
        p.drawPixmap(0, 0, (*maskImage).first);
    }
    p.end();
    return result;
}

static QPixmap masksToIcon(const MasksAndColors &masks, const QPixmap &combinedMask, Icon::Style style)
{
    QPixmap result(combinedMask.size());
    result.setDevicePixelRatio(combinedMask.devicePixelRatio());
    result.fill(Qt::transparent);
    QPainter p(&result);

    for (MasksAndColors::const_iterator maskImage = masks.constBegin();
         maskImage != masks.constEnd(); ++maskImage) {
        if (maskImage != masks.constBegin()) {
            // Punch a transparent outline around an overlay.
            p.save();
            p.setOpacity(0.4);
            p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
            smearPixmap(&p, maskToColorAndAlpha((*maskImage).first, Qt::white), 0.5);
            p.restore();
        }
        p.drawPixmap(0, 0, maskToColorAndAlpha((*maskImage).first, (*maskImage).second));
    }

    if (style == Icon::Style::TintedWithShadow) {
        const QPixmap shadowMask = maskToColorAndAlpha(combinedMask, Qt::black);
        p.setCompositionMode(QPainter::CompositionMode_DestinationOver);
        p.setOpacity(0.05);
        p.drawPixmap(QPointF(0, -0.501), shadowMask);
        p.drawPixmap(QPointF(-0.501, 0), shadowMask);
        p.drawPixmap(QPointF(0.5, 0), shadowMask);
        p.drawPixmap(QPointF(0.5, 0.5), shadowMask);
        p.drawPixmap(QPointF(-0.501, 0.5), shadowMask);
        p.setOpacity(0.2);
        p.drawPixmap(0, 1, shadowMask);
    }

    p.end();

    return result;
}

static QPixmap combinedPlainPixmaps(const QVector<IconMaskAndColor> &images)
{
    QPixmap result(StyleHelper::dpiSpecificImageFile(images.first().first));
    auto pixmap = images.constBegin();
    pixmap++;
    for (;pixmap != images.constEnd(); ++pixmap) {
        const QPixmap overlay(StyleHelper::dpiSpecificImageFile((*pixmap).first));
        result.paintEngine()->painter()->drawPixmap(0, 0, overlay);
    }
    return result;
}

Icon::Icon()
{
}

Icon::Icon(std::initializer_list<IconMaskAndColor> args, Style style)
    : QVector<IconMaskAndColor>(args)
    , m_style(style)
{
}

Icon::Icon(const QString &imageFileName)
    : m_style(Style::Plain)
{
    append({imageFileName, Theme::Color(-1)});
}

QIcon Icon::icon() const
{
    if (isEmpty()) {
        return QIcon();
    } else if (m_style == Style::Plain) {
        return QIcon(combinedPlainPixmaps(*this));
    } else {
        QIcon result;
        const MasksAndColors masks = masksAndColors(*this, qRound(qApp->devicePixelRatio()));
        const QPixmap combinedMask = Utils::combinedMask(masks);
        result.addPixmap(masksToIcon(masks, combinedMask, m_style));

        QColor disabledColor = creatorTheme()->palette().mid().color();
        disabledColor.setAlphaF(0.6);
        result.addPixmap(maskToColorAndAlpha(combinedMask, disabledColor), QIcon::Disabled);
        return result;
    }
}

QPixmap Icon::pixmap() const
{
    if (isEmpty()) {
        return QPixmap();
    } else if (m_style == Style::Plain) {
        return combinedPlainPixmaps(*this);
    } else {
        const MasksAndColors masks =
                masksAndColors(*this, qRound(qApp->devicePixelRatio()));
        const QPixmap combinedMask = Utils::combinedMask(masks);
        return masksToIcon(masks, combinedMask, m_style);
    }
}

QString Icon::imageFileName() const
{
    QTC_ASSERT(length() == 1, return QString());
    return first().first;
}

QIcon Icon::sideBarIcon(const Icon &classic, const Icon &flat)
{
    QIcon result;
    if (creatorTheme()->flag(Theme::FlatSideBarIcons)) {
        result = flat.icon();
    } else {
        const QPixmap pixmap = classic.pixmap();
        result.addPixmap(pixmap);
        // Ensure that the icon contains a disabled state of that size, since
        // Since we have icons with mixed sizes (e.g. DEBUG_START), and want to
        // avoid that QIcon creates scaled versions of missing QIcon::Disabled
        // sizes.
        result.addPixmap(StyleHelper::disabledSideBarIcon(pixmap), QIcon::Disabled);
    }
    return result;
}

QIcon Icon::modeIcon(const Icon &classic, const Icon &flat, const Icon &flatActive)
{
    QIcon result = sideBarIcon(classic, flat);
    if (creatorTheme()->flag(Theme::FlatSideBarIcons))
        result.addPixmap(flatActive.pixmap(), QIcon::Active);
    return result;
}

QIcon Icon::combinedIcon(const QList<QIcon> &icons)
{
    QIcon result;
    QWindow *window = QApplication::allWidgets().first()->windowHandle();
    for (const QIcon &icon: icons)
        for (const QIcon::Mode mode: {QIcon::Disabled, QIcon::Normal})
            for (const QSize &size: icon.availableSizes(mode))
                result.addPixmap(icon.pixmap(window, size, mode), mode);
    return result;
}

} // namespace Utils
