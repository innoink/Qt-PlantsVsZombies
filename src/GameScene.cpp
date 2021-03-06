//
// Created by sun on 8/26/16.
//

#include <QtAlgorithms>
#include <stdlib.h>
#include <math.h>
#include "GameScene.h"
#include "MainView.h"
#include "ImageManager.h"
#include "Timer.h"
#include "Plant.h"
#include "Zombie.h"
#include "GameLevelData.h"
#include "MouseEventPixmapItem.h"
#include "PlantCardItem.h"
#include "Animate.h"

GameScene::GameScene(GameLevelData *gameLevelData)
        : QGraphicsScene(0, 0, 900, 600),
          gameLevelData(gameLevelData),
          background(new QGraphicsPixmapItem(gImageCache->load(gameLevelData->backgroundImage))),
          gameGroup(new QGraphicsItemGroup),
          infoText(new QGraphicsSimpleTextItem),
          infoTextGroup(new QGraphicsRectItem(0, 0, 900, 50)),
          menuGroup(new MouseEventPixmapItem(gImageCache->load("interface/Button.png"))),
          sunNumText(new QGraphicsSimpleTextItem(QString::number(gameLevelData->sunNum))),
          sunNumGroup(new QGraphicsPixmapItem(gImageCache->load("interface/SunBack.png"))),
          selectCardButtonReset(new MouseEventPixmapItem(gImageCache->load("interface/SelectCardButton.png"))),
          selectCardButtonOkay(new MouseEventPixmapItem(gImageCache->load("interface/SelectCardButton.png"))),
          selectCardTextReset(new QGraphicsSimpleTextItem(tr("Reset"))),
          selectCardTextOkay(new QGraphicsSimpleTextItem(tr("Go"))),
          selectingPanel(new QGraphicsPixmapItem(gImageCache->load("interface/SeedChooser_Background.png"))),
          cardPanel(new QGraphicsItemGroup),
          movePlantAlpha(new QGraphicsPixmapItem),
          movePlant(new QGraphicsPixmapItem),
          imgGrowSoil(new MoviePixmapItem("interface/GrowSoil.gif")),
          imgGrowSpray(new MoviePixmapItem("interface/GrowSpray.gif")),
          flagMeter(new FlagMeter(gameLevelData)),
          coordinate(gameLevelData->coord),
          choose(0), sunNum(gameLevelData->sunNum),
          waveTimer(nullptr), waveNum(0)
{
    // Process ProtoTypes
    for (const auto &eName: gameLevelData->pName)
        plantProtoTypes.insert(eName, PlantFactory(eName));
    for (const auto &eName: gameLevelData->zName)
        zombieProtoTypes.insert(eName, ZombieFactory(eName));
    // z-value -- 0: normal 1: tooltip 2: dialog
    // Background (parent of the zombies displayed on the road)
    addItem(background);
    if (gameLevelData->showScroll) {
        QList<qreal> yPos;
        QList<Zombie *> zombies;
        for (const auto &zombieData: gameLevelData->zombieData) {
            Zombie *item = zombieProtoTypes[zombieData.eName];
            if(item->canDisplay) {
                for (int i = 0; i < zombieData.num; ++i) {
                    yPos.push_back(qFloor(100 +  qrand() % 400));
                    zombies.push_back(item);
                }
            }
        }
        qSort(yPos.begin(), yPos.end());
        std::random_shuffle(zombies.begin(), zombies.end());
        for (int i = 0; i < zombies.size(); ++i) {
            MoviePixmapItem *pixmap = new MoviePixmapItem(zombies[i]->standGif);
            QSizeF size = pixmap->boundingRect().size();
            pixmap->setPos(qFloor(1115 + qrand() % 200) - size.width() * 0.5, yPos[i] - size.width() * 0.5);
            pixmap->setParentItem(background);
        }
    }
    // Plants, zombies and sun
    addItem(gameGroup);
    // Information text
    infoText->setBrush(Qt::white);
    infoText->setFont(QFont("SimHei", 16, QFont::Bold));
    infoText->setParentItem(infoTextGroup);
    infoTextGroup->setPos(0, 500);
    infoTextGroup->setPen(Qt::NoPen);
    infoTextGroup->setBrush(QColor::fromRgb(0x5b432e));
    infoTextGroup->setOpacity(0);
    addItem(infoTextGroup);
    // Menu
    QGraphicsSimpleTextItem *menuText = new QGraphicsSimpleTextItem(tr("Menu"));
    menuText->setBrush(QColor::fromRgb(0x00cb08));
    menuText->setFont(QFont("SimHei", 12, QFont::Bold));
    menuText->setParentItem(menuGroup);
    menuText->setPos(SizeToPoint(menuGroup->boundingRect().size() - menuText->boundingRect().size()) / 2);
    menuGroup->setPos(sceneRect().topRight() - SizeToPoint(menuGroup->boundingRect().size()));
    menuGroup->setCursor(Qt::PointingHandCursor);
    addItem(menuGroup);
    // Sun number
    sunNumText->setFont(QFont("Verdana", 16, QFont::Bold));
    QSizeF sunNumTextSize = sunNumText->boundingRect().size();
    sunNumText->setPos(76 - sunNumTextSize.width() / 2,
                       (sunNumGroup->boundingRect().height() - sunNumTextSize.height()) / 2);
    sunNumText->setParentItem(sunNumGroup);
    sunNumGroup->setPos(100, 560);
    sunNumGroup->setVisible(false);
    addItem(sunNumGroup);
    // Select Card
    if (gameLevelData->canSelectCard && gameLevelData->maxSelectedCards > 0) {
        // Title
        QGraphicsSimpleTextItem *selectCardTitle = new QGraphicsSimpleTextItem(tr("Choose your cards"));
        selectCardTitle->setBrush(QColor::fromRgb(0xf0c060));
        selectCardTitle->setFont(QFont("NSimSun", 12, QFont::Bold));
        QSizeF selectCardTitleSize = selectCardTitle->boundingRect().size();
        selectCardTitle->setPos((selectingPanel->boundingRect().width() - selectCardTitleSize.width()) / 2,
                                15 - selectCardTitleSize.height() / 2);
        selectCardTitle->setParentItem(selectingPanel);
        // Reset button
        selectCardTextReset->setBrush(QColor::fromRgb(0x808080));
        selectCardTextReset->setFont(QFont("SimHei", 12, QFont::Bold));
        selectCardTextReset->setPos(SizeToPoint(selectCardButtonReset->boundingRect().size()
                                                - selectCardTextReset->boundingRect().size()) / 2);
        selectCardTextReset->setParentItem(selectCardButtonReset);
        selectCardButtonReset->setPos(162, 500);
        selectCardButtonReset->setEnabled(false);
        selectCardButtonReset->setParentItem(selectingPanel);
        // Okay button
        selectCardTextOkay->setBrush(QColor::fromRgb(0x808080));
        selectCardTextOkay->setFont(QFont("SimHei", 12, QFont::Bold));
        selectCardTextOkay->setPos(SizeToPoint(selectCardButtonOkay->boundingRect().size()
                                               - selectCardTextOkay->boundingRect().size()) / 2);
        selectCardTextOkay->setParentItem(selectCardButtonOkay);
        selectCardButtonOkay->setPos(237, 500);
        selectCardButtonOkay->setEnabled(false);
        selectCardButtonOkay->setParentItem(selectingPanel);
        // Plant cards to select
        int cardIndex = 0;
        for (auto item: plantProtoTypes.values()) {
            if (!item->canSelect) continue;
            // Plant cards
            PlantCardItem *plantCardItem = new PlantCardItem(item, true);
            plantCardItem->setPos(15 + cardIndex % 6 * 72, 40 + cardIndex / 6 * 50);
            plantCardItem->setCursor(Qt::PointingHandCursor);
            plantCardItem->setParentItem(selectingPanel);
            // Tooltip
            QString tooltipText = "<b>" + item->cName + "</b><br />" +
                    QString(tr("Cool down: %1s")).arg(item->coolTime) + "<br />";
            if (gameLevelData->dKind != 0 && item->night)
                tooltipText += "<span style=\"color:#F00\">" + tr("Nocturnal - sleeps during day") + "</span><br>";
            tooltipText += item->toolTip;
            TooltipItem *tooltipItem = new TooltipItem(tooltipText);
            tooltipItem->setVisible(false);
            tooltipItem->setOpacity(0.9);
            tooltipItem->setZValue(1);
            addItem(tooltipItem);
            // Displaying tooltip when hovering
            QPointF posDelta(5, 15);
            connect(plantCardItem, &PlantCardItem::hoverEntered, [tooltipItem, posDelta](QGraphicsSceneHoverEvent *event) {
                tooltipItem->setPos(event->scenePos() + posDelta);
                tooltipItem->setVisible(true);
            });
            connect(plantCardItem, &PlantCardItem::hoverMoved, [tooltipItem, posDelta](QGraphicsSceneHoverEvent *event) {
                tooltipItem->setPos(event->scenePos() + posDelta);
            });
            connect(plantCardItem, &PlantCardItem::hoverLeft, [tooltipItem, posDelta](QGraphicsSceneHoverEvent *event) {
                tooltipItem->setPos(event->scenePos() + posDelta);
                tooltipItem->setVisible(false);
            });
            // Move and scale to selected card panel when clicking
            connect(plantCardItem, &PlantCardItem::clicked, [this, item, plantCardItem] {
                // Check
                if (!plantCardItem->isChecked()) return;
                int count = selectedPlantArray.size();
                if (this->gameLevelData->maxSelectedCards > 0 && count >= this->gameLevelData->maxSelectedCards)
                    return;
                // Okay
                plantCardItem->setChecked(false);
                PlantCardItem *selectedPlantCardItem = new PlantCardItem(item, true);
                selectedPlantCardItem->setPos(plantCardItem->scenePos());
                cardPanel->addToGroup(selectedPlantCardItem);
                selectedPlantArray.push_back(item);
                if (count == 0) {
                    selectCardTextReset->setBrush(QColor::fromRgb(0xf0c060));
                    selectCardTextOkay->setBrush(QColor::fromRgb(0xf0c060));
                    selectCardButtonReset->setEnabled(true);
                    selectCardButtonOkay->setEnabled(true);
                    selectCardButtonReset->setCursor(Qt::PointingHandCursor);
                    selectCardButtonOkay->setCursor(Qt::PointingHandCursor);
                }
                Animate(selectedPlantCardItem).move(QPointF(0, 60 * count)).scale(1).speed(1).replace().finish();
                // Move and scale back to unselected card panel when clicking
                QSharedPointer<QMetaObject::Connection> deselectConnnection(new QMetaObject::Connection), resetConnnection(new QMetaObject::Connection);
                auto deselectFunctor = [this, item, plantCardItem, selectedPlantCardItem, deselectConnnection, resetConnnection] {
                    disconnect(*deselectConnnection);
                    disconnect(*resetConnnection);
                    cardPanel->removeFromGroup(selectedPlantCardItem);
                    selectedPlantArray.removeOne(item);
                    if (selectedPlantArray.size() == 0) {
                        selectCardTextReset->setBrush(QColor::fromRgb(0x808080));
                        selectCardTextOkay->setBrush(QColor::fromRgb(0x808080));
                        selectCardButtonReset->setEnabled(false);
                        selectCardButtonOkay->setEnabled(false);
                        selectCardButtonReset->setCursor(Qt::ArrowCursor);
                        selectCardButtonOkay->setCursor(Qt::ArrowCursor);
                    }
                    Animate(selectedPlantCardItem).move(plantCardItem->scenePos()).scale(0.7).speed(1).replace().finish(
                        [plantCardItem, selectedPlantCardItem] {
                            plantCardItem->setChecked(true);
                            delete selectedPlantCardItem;
                    });
                };
                *deselectConnnection = connect(selectedPlantCardItem, &PlantCardItem::clicked, [this, selectedPlantCardItem, deselectFunctor] {
                    QList<QGraphicsItem *> selectedCards = cardPanel->childItems();
                    for (int i = qFind(selectedCards, selectedPlantCardItem) - selectedCards.begin() + 1; i != selectedCards.size(); ++i)
                        Animate(selectedCards[i]).move(QPointF(0, 60 * (i - 1))).speed(1).replace().finish();
                    deselectFunctor();
                });
                *resetConnnection = connect(selectCardButtonReset, &MouseEventPixmapItem::clicked, deselectFunctor);
            });
            ++cardIndex;
        }
        selectingPanel->setPos(100, -selectingPanel->boundingRect().height());
        addItem(selectingPanel);
    }
    // Selected card
    cardPanel->setHandlesChildEvents(false);
    addItem(cardPanel);
    // Move plant
    movePlantAlpha->setOpacity(0.4);
    movePlantAlpha->setVisible(false);
    movePlantAlpha->setZValue(30);
    gameGroup->addToGroup(movePlantAlpha);
    movePlant->setVisible(false);
    movePlant->setZValue(254);
    addItem(movePlant);
    // Effects for growing plants
    imgGrowSoil->setVisible(false);
    imgGrowSoil->setZValue(50);
    gameGroup->addToGroup(imgGrowSoil);
    imgGrowSpray->setVisible(false);
    imgGrowSpray->setZValue(50);
    gameGroup->addToGroup(imgGrowSpray);
    // Flag progress
    flagMeter->setPos(700, 610);
    addItem(flagMeter);
}

GameScene::~GameScene()
{
    for (auto i: plantProtoTypes.values())
        delete i;
    for (auto i: zombieProtoTypes.values())
        delete i;
    for (auto i: plantInstances)
        delete i;
    for (auto i: zombieInstances)
        delete i;

    delete gameLevelData;
}

void GameScene::setInfoText(const QString &text)
{
    if (text.isEmpty())
        Animate(infoTextGroup).fade(0).duration(200).finish();
    else {
        infoText->setText(text);
        infoText->setPos(SizeToPoint(infoTextGroup->boundingRect().size() - infoText->boundingRect().size()) / 2);
        Animate(infoTextGroup).fade(0.8).duration(200).finish();
    }
}

void GameScene::loadReady()
{
    gMainView->getMainWindow()->setWindowTitle(tr("Plants vs. Zombies") + " - " + gameLevelData->cName);
    if (!gameLevelData->showScroll)
        background->setPos(-115, 0);
    gameLevelData->loadAccess(this);
}

void GameScene::loadAcessFinished()
{
    if (!gameLevelData->showScroll || !gameLevelData->canSelectCard) {
        for (auto item: plantProtoTypes.values()) {
            if (item->canSelect) {
                selectedPlantArray.push_back(item);
                if (gameLevelData->maxSelectedCards > 0 && selectedPlantArray.size() >= gameLevelData->maxSelectedCards)
                    break;
            }
        }
    }
    if (gameLevelData->showScroll) {
        setInfoText(QString(tr("%1\' house")).arg(QSettings().value("Global/Username").toString()));
        (new Timer(this, 1000, [this]{
            setInfoText("");
            for (auto zombie: background->childItems())
                static_cast<MoviePixmapItem *>(zombie)->start();
            Animate(background).move(QPointF(-500, 0)).speed(0.5).finish([this] {
                Animate(menuGroup).move(QPointF(sceneRect().topRight() - QPointF(menuGroup->boundingRect().width(), 0))).speed(0.5).finish();
                auto scrollBack = [this] {
                    Animate(background).move(QPointF(-115, 0)).speed(0.5).finish([this] {
                        for (auto zombie: background->childItems())
                            delete zombie;
                        letsGo();
                    });
                };
                if (gameLevelData->canSelectCard) {
                    Animate(selectingPanel).move(QPointF(100, 0)).speed(3).finish([this] { sunNumGroup->setVisible(true); });
                    connect(selectCardButtonOkay, &MouseEventPixmapItem::clicked, [this, scrollBack] {
                        sunNumGroup->setVisible(false);
                        for (auto card: cardPanel->childItems()) {
                            card->setCursor(Qt::ArrowCursor);
                            card->setEnabled(false);
                        }
                        Animate(selectingPanel).move(QPointF(100, -selectingPanel->boundingRect().height())).speed(3).finish(scrollBack);
                    });
                }
                else {
                    (new Timer(this, 1000, scrollBack))->start();
                }
            });
        }))->start();
    }
    else {
        Animate(menuGroup).move(QPointF(sceneRect().topRight() - QPointF(menuGroup->boundingRect().width(), 0))).speed(0.5).finish();
        letsGo();
    }
}


void GameScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    QGraphicsScene::mouseMoveEvent(mouseEvent);
    emit mouseMove(mouseEvent);
}

void GameScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    QGraphicsScene::mousePressEvent(mouseEvent);
    emit mousePress(mouseEvent);
}

GameLevelData *GameScene::getGameLevelData() const
{
    return gameLevelData;
}

void GameScene::letsGo()
{
    sunNumGroup->setPos(105, -sunNumGroup->boundingRect().height());
    sunNumGroup->setVisible(true);
    Animate(sunNumGroup).move(QPointF(105, 0)).speed(0.5).finish();
    if (!gameLevelData->showScroll || !gameLevelData->canSelectCard) {
        cardPanel->setPos(-100, 0);
        Animate(cardPanel).move(QPointF(0, 0)).speed(0.5).finish();
    }
    for (auto i: cardPanel->childItems())
        delete i;
    for (int i = 0; i < selectedPlantArray.size(); ++i) {
        auto &item = selectedPlantArray[i];

        PlantCardItem *plantCardItem = new PlantCardItem(item);
        plantCardItem->setChecked(false);
        cardPanel->addToGroup(plantCardItem);
        plantCardItem->setPos(0, i * 60);
        TooltipItem *tooltipItem = new TooltipItem("");
        tooltipItem->setVisible(false);
        tooltipItem->setOpacity(0.9);
        tooltipItem->setZValue(1);
        addItem(tooltipItem);

        connect(plantCardItem, &PlantCardItem::hoverEntered, [this, tooltipItem](QGraphicsSceneHoverEvent *event) {
            if (choose) return;
            tooltipItem->setPos(event->scenePos() + QPointF(5, 15));
            tooltipItem->setVisible(true);
        });
        connect(plantCardItem, &PlantCardItem::hoverMoved, [this, tooltipItem](QGraphicsSceneHoverEvent *event) {
            if (choose) return;
            tooltipItem->setPos(event->scenePos() + QPointF(5, 15));
        });
        connect(plantCardItem, &PlantCardItem::hoverLeft, [this, tooltipItem](QGraphicsSceneHoverEvent *event) {
            if (choose) return;
            tooltipItem->setPos(event->scenePos() + QPointF(5, 15));
            tooltipItem->setVisible(false);
        });

        cardGraphics.push_back({ plantCardItem, tooltipItem });
        cardReady.push_back({ false, false });
        updateTooltip(i);
    }
    // All excluded mousePress or clicked must be triggered from one object to avoid duplicated triggering.
    connect(this, &GameScene::mousePress, [this](QGraphicsSceneMouseEvent *event) {
        if (choose) return;
        for (int i = 0; i < selectedPlantArray.size(); ++i) {
            if (!cardReady[i].cool || !cardReady[i].sun ||
                !cardGraphics[i].plantCard->contains(event->scenePos() - cardGraphics[i].plantCard->scenePos()))
                continue;
            cardGraphics[i].tooltip->setVisible(false);
            auto &item = selectedPlantArray[i];
            QPixmap staticGif = gImageCache->load(item->staticGif);
            QPointF delta = QPointF(-0.5 * (item->beAttackedPointL + item->beAttackedPointR), 20 - staticGif.height());
            movePlantAlpha->setPixmap(staticGif);
            movePlant->setPixmap(staticGif);
            movePlant->setPos(event->scenePos() + delta);
            movePlant->setVisible(true);

            choose = 1;
            QSharedPointer<QMetaObject::Connection> moveConnection(new QMetaObject::Connection()), clickConnection(new QMetaObject::Connection());
            *moveConnection = connect(this, &GameScene::mouseMove, [this, delta, item](QGraphicsSceneMouseEvent *e) {
                movePlant->setPos(e->scenePos() + delta);
                auto xPair = coordinate.choosePlantX(e->scenePos().x()), yPair = coordinate.choosePlantY(e->scenePos().y());
                if (item->canGrow(*this, xPair.second, yPair.second)) {
                    movePlantAlpha->setVisible(true);
                    movePlantAlpha->setPos(xPair.first + item->getDX(), yPair.first + item->getDY(xPair.second, yPair.second) - item->height);
                }
                else
                    movePlantAlpha->setVisible(false);
            });
            *clickConnection = connect(this, &GameScene::mousePress, [this, i, moveConnection, clickConnection, item](QGraphicsSceneMouseEvent *e) {
                choose = 0;
                movePlant->setVisible(false);
                movePlantAlpha->setVisible(false);
                auto xPair = coordinate.choosePlantX(e->scenePos().x()), yPair = coordinate.choosePlantY(e->scenePos().y());
                if (item->canGrow(*this, xPair.second, yPair.second)) {
                    MoviePixmapItem *growGif;
                    if (gameLevelData->LF[yPair.second] == 1)
                        growGif = imgGrowSoil;
                    else
                        growGif = imgGrowSpray;
                    growGif->setPos(xPair.first - 30, yPair.first - 30);
                    growGif->setVisible(true);
                    growGif->start();
                    QSharedPointer<QMetaObject::Connection> connection(new QMetaObject::Connection());
                    *connection.data() = connect(growGif, &MoviePixmapItem::finished, [growGif, connection]{
                        growGif->setVisible(false);
                        growGif->reset();
                        disconnect(*connection.data());
                    });
                    PlantInstance *plantInstance = PlantInstanceFactory(item);
                    plantInstance->birth(*this, xPair.first, yPair.first, xPair.second, yPair.second);
                    plantInstances.push_back(plantInstance);
                    doCoolTime(i);
                    sunNum -= item->sunNum;
                    updateSunNum();
                }
                disconnect(*moveConnection.data());
                disconnect(*clickConnection.data());
            });
        }
    });
    gameLevelData->startGame(this);
}

void GameScene::beginCool()
{
    for (int i = 0; i < selectedPlantArray.size(); ++i) {
        auto &item = selectedPlantArray[i];
        auto &plantCardItem = cardGraphics[i].plantCard;
        if (item->coolTime < 7.6) {
            plantCardItem->setPercent(1.0);
            cardReady[i].cool = true;
            if (item->sunNum <= sunNum) {
                cardReady[i].sun = true;
                plantCardItem->setChecked(true);
            }
            updateTooltip(i);
            continue;
        }
        doCoolTime(i);
    }
}

void GameScene::updateTooltip(int index)
{
    auto &item = selectedPlantArray[index];
    QString text = "<b>" + item->cName + "</b><br />" +
                   QString(tr("Cool down: %1s")).arg(item->coolTime) + "<br />";
    text += item->toolTip;
    if (!cardReady[index].cool)
        text += "<br><span style=\"color:#f00\">" + tr("Rechanging...") + "</span>";
    if (!cardReady[index].sun)
        text += "<br><span style=\"color:#f00\">" + tr("Not enough sun!") + "</span>";
    cardGraphics[index].tooltip->setText(text);
}

void GameScene::beginSun(int sunNum)
{
    MoviePixmapItem *sunGif = new MoviePixmapItem("interface/Sun.gif");
    if (sunNum == 15)
        sunGif->setScale(46.0 / 79.0);
    else if (sunNum != 25)
        sunGif->setScale(100.0 / 79.0);
    double toX = coordinate.getX(1 + qrand() % coordinate.colCount()),
           toY = coordinate.getY(1 + qrand() % coordinate.rowCount());
    sunGif->setPos(toX, -100);
    sunGif->setZValue(2);
    sunGif->setOpacity(0.8);
    sunGif->setCursor(Qt::PointingHandCursor);
    sunGif->start();
    addItem(sunGif);
    QSharedPointer<QTimer *> timer(new QTimer *(nullptr));
    QSharedPointer<QMetaObject::Connection> connection(new QMetaObject::Connection());
    Animate(sunGif).move(QPointF(toX, toY - 53)).speed(0.04).finish([this, sunGif, timer, connection](bool finished) {
        if (finished) {
            (*timer = new Timer(this, 8000, [this, sunGif, connection] {
                disconnect(*connection);
                sunGif->setCursor(Qt::ArrowCursor);
                Animate(sunGif).fade(0).duration(500).finish([sunGif] {
                    delete sunGif;
                });
            }))->start();
        }
    });

    *connection = connect(sunGif, &MoviePixmapItem::click, [this, sunGif, sunNum, timer] {
        if (choose != 0) return;
        if (*timer) {
            (*timer)->stop();
            delete *timer;
        }
        Animate(sunGif).finish().move(QPointF(100, 0)).speed(1).scale(34.0 / 79.0).finish([this, sunGif, sunNum] {
            delete sunGif;
            this->sunNum += sunNum;
            updateSunNum();
        });
    });
    (new Timer(this, (qrand() % 9000 + 3000), [this, sunNum] { beginSun(sunNum); }))->start();
}

void GameScene::doCoolTime(int index)
{
    auto &item = selectedPlantArray[index];
    cardGraphics[index].plantCard->setPercent(0);
    cardGraphics[index].plantCard->setChecked(false);
    if (cardReady[index].cool) {
        cardReady[index].cool = false;
        updateTooltip(index);
    }
    (new TimeLine(this, qRound(item->coolTime * 1000), 20, [this, index](qreal x) {
        cardGraphics[index].plantCard->setPercent(x);
    }, [this, index] {
        cardReady[index].cool = true;
        if (cardReady[index].sun)
            cardGraphics[index].plantCard->setChecked(true);
        updateTooltip(index);
    }))->start();
}

void GameScene::updateSunNum()
{
    sunNumText->setText(QString::number(sunNum));
    QSizeF sunNumTextSize = sunNumText->boundingRect().size();
    sunNumText->setPos(76 - sunNumTextSize.width() / 2, (sunNumGroup->boundingRect().height() - sunNumTextSize.height()) / 2);
    for (int i = 0; i < selectedPlantArray.size(); ++i) {
        auto &item = selectedPlantArray[i];
        auto &plantCardItem = cardGraphics[i].plantCard;
        if (item->sunNum <= sunNum) {
            if (!cardReady[i].sun) {
                cardReady[i].sun = true;
                updateTooltip(i);
            }
            if (cardReady[i].cool)
                plantCardItem->setChecked(true);
        }
        else {
            if (cardReady[i].sun) {
                cardReady[i].sun = false;
                updateTooltip(i);
            }
            plantCardItem->setChecked(false);
        }
    }
}

QPointF GameScene::SizeToPoint(const QSizeF &size)
{
    return QPointF(size.width(), size.height());
}

void GameScene::customSpecial(const QString &name, int col, int row)
{
    if (plantProtoTypes.find(name) == plantProtoTypes.end())
        plantProtoTypes.insert(name, PlantFactory(name));
    PlantInstance *plantInstance = PlantInstanceFactory(plantProtoTypes[name]);
    plantInstance->birth(*this, coordinate.getX(col), coordinate.getY(row), col, row);
    plantInstances.push_back(plantInstance);
}

void GameScene::addToGame(QGraphicsItem *item)
{
    gameGroup->addToGroup(item);
}

void GameScene::beginZombies()
{
    Animate(flagMeter).move(QPointF(700, 560)).speed(0.5).finish();
    advanceFlag();
}

void GameScene::prepareGrowPlants(std::function<void(void)> functor)
{
    QPixmap imgPrepareGrowPlants = gImageCache->load("interface/PrepareGrowPlants.png");
    QGraphicsPixmapItem *imgPrepare = new QGraphicsPixmapItem(imgPrepareGrowPlants.copy(0, 0, 255, 108)),
            *imgGrow    = new QGraphicsPixmapItem(imgPrepareGrowPlants.copy(0, 108, 255, 108)),
            *imgPlants  = new QGraphicsPixmapItem(imgPrepareGrowPlants.copy(0, 216, 255, 108));
    QPointF pos = SizeToPoint(sceneRect().size() - imgPrepare->boundingRect().size()) / 2;
    imgPrepare->setPos(pos);
    imgGrow->setPos(pos);
    imgPlants->setPos(pos);
    imgPrepare->setZValue(1);
    imgGrow->setZValue(1);
    imgPlants->setZValue(1);
    imgPrepare->setVisible(false);
    imgGrow->setVisible(false);
    imgPlants->setVisible(false);
    addItem(imgPrepare);
    addItem(imgGrow);
    addItem(imgPlants);
    imgPrepare->setVisible(true);
    (new Timer(this, 600, [this, imgPrepare, imgGrow, imgPlants, functor] {
        delete imgPrepare;
        imgGrow->setVisible(true);
        (new Timer(this, 400, [this, imgGrow, imgPlants, functor] {
            delete imgGrow;
            imgPlants->setVisible(true);
            (new Timer(this, 1000, [this, imgPlants, functor] {
                delete imgPlants;
                functor();
            }))->start();
        }))->start();
    }))->start();
}

void GameScene::advanceFlag()
{
    ++waveNum;
    flagMeter->updateFlagZombies(waveNum);
    if (waveNum < gameLevelData->flagNum) {
        auto iter = gameLevelData->flagToMonitor.find(waveNum);
        if (iter != gameLevelData->flagToMonitor.end())
            (new Timer(this, 16900, [this, iter] { (*iter)(this); }))->start();
        (waveTimer = new Timer(this, 1000/*19900*/, [this] { advanceFlag(); }))->start();
    }
    auto &flagToSumNum = gameLevelData->flagToSumNum;
    selectFlagZombie(flagToSumNum.second[qLowerBound(flagToSumNum.first, waveNum) - flagToSumNum.first.begin()]);
}

void GameScene::plantDie(PlantInstance *plant)
{
    int i = plantInstances.indexOf(plant);
    delete plantInstances[i];
    plantInstances.removeAt(i);
}


void GameScene::zombieDie(ZombieInstance *zombie)
{
    int i = zombieInstances.indexOf(zombie);
    delete zombieInstances[i];
    zombieInstances.removeAt(i);
    if (zombieInstances.isEmpty()) {
        waveTimer->stop();
        (new Timer(this, 5000, [this] { advanceFlag(); }))->start();
    }
}

void GameScene::selectFlagZombie(int levelSum)
{
    qDebug() << "Wave: " << waveNum << "\tLevel Sum: " << levelSum;
    QList<Zombie *> zombies, zombiesCandidate;
    if (gameLevelData->largeWaveFlag.contains(waveNum)) {
        // Add flag zombie (j--)
    }
    for (const auto &zombieData: gameLevelData->zombieData) {
        if (zombieData.flagList.contains(levelSum)) {
            Zombie *item = zombieProtoTypes[zombieData.eName];
            levelSum -= item->level;
            zombies.push_back(item);
        }
        if (zombieData.firstFlag <= levelSum) {
            Zombie *item = zombieProtoTypes[zombieData.eName];
            for (int i = 0; i < zombieData.num; ++i)
                zombiesCandidate.push_back(item);
        }
    }
    qSort(zombiesCandidate.begin(), zombiesCandidate.end(), [](Zombie *a, Zombie *b) { return a->level < b->level; });
    while (levelSum > 0) {
        while (zombiesCandidate.last()->level > levelSum)
            zombiesCandidate.pop_back();
        Zombie *item = zombiesCandidate[qrand() % zombiesCandidate.size()];
        levelSum -= item->level;
        zombies.push_back(item);
    }
    for (auto item: zombies)
        qDebug() << "    " << item->cName;
}

FlagMeter::FlagMeter(GameLevelData *gameLevelData)
    : flagNum(gameLevelData->flagNum),
      flagHeadStep(140.0 / (flagNum - 1)),
      flagMeterEmpty(gImageCache->load("interface/FlagMeterEmpty.png")),
      flagMeterFull(gImageCache->load("interface/FlagMeterFull.png")),
      flagTitle(new QGraphicsPixmapItem(gImageCache->load("interface/FlagMeterLevelProgress.png"))),
      flagHead(new QGraphicsPixmapItem(gImageCache->load("interface/FlagMeterParts1.png")))
{
    setPixmap(flagMeterEmpty);

    flagTitle->setPos(35, 12);
    flagTitle->setParentItem(this);
    for (auto i: gameLevelData->largeWaveFlag) {
        QGraphicsPixmapItem *flag = new QGraphicsPixmapItem(gImageCache->load("interface/FlagMeterParts2.png"));
        flag->setPos(150 - (i - 1) * flagHeadStep, -3);
        flag->setParentItem(this);
        flags.insert(i, flag);
    }
    flagHead->setPos(139, -4);
    flagHead->setParentItem(this);
    updateFlagZombies(1.0);
}

void FlagMeter::updateFlagZombies(int flagZombies)
{
    auto iter = flags.find(flagZombies);
    if (iter != flags.end())
        iter.value()->setY(-12);
    if (flagZombies < flagNum) {
        qreal x = 150 - (flagZombies - 1) * flagHeadStep;
        flagHead->setPos(x - 11, -4);
        QPixmap flagMeter(flagMeterFull);
        QPainter p(&flagMeter);
        p.drawPixmap(0, 0, flagMeterEmpty.copy(0, 0, qRound(x), 21));
        setPixmap(flagMeter);
    }
    else {
        flagHead->setPos(-1, -3);
        setPixmap(flagMeterFull);
    }
}
