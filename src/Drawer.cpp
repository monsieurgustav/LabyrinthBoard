//
//  Drawer.cpp
//  Labyrinth
//
//  Created by Guillaume Buisson on 05/10/13.
//
//

#include "Drawer.h"

#include "Board.h"
#include "Actor.h"
#include "Helpers.h"

#include "cinder/Timeline.h"


void Drawer::setTile(int tileId, ci::gl::TextureRef image, int height,
                     int line, int begin, int end,
                     float duration)
{
    Tile tmp = {image, height, line, begin, end, begin};
    Tile & tile = mIdToTile[tileId] = tmp;
    // vs2010 does not support generalized initializer.
    // Tile & tile = mIdToTile[tileId] = {image, height, begin, end, begin};
    if(end > begin+1)
    {
        mTimeline->apply(&tile.current, end, duration).loop();
    }
}

static ci::Area pickTileIndex(const ci::Area & imageSize,
                              int tileWidth, int tileHeight,
                              int tileLine, int tileIndex)
{
    const int tileCountInRow = imageSize.getWidth() / tileWidth;
    tileIndex %= tileCountInRow;
    return ci::Area(tileIndex*tileWidth, (tileLine+1)*tileWidth-tileHeight,
                    (tileIndex+1)*tileWidth, (tileLine+1)*tileWidth);
}

/**
 * @brief Compute the tile offset based on the player position
 *
 * The given offset takes the board size into account in order to not step
 * out of it.
 */
static ci::Vec2i positionToScrollOffset(ci::Vec2i position,
                                        ci::Vec2i viewSize,
                                        ci::Vec2i boardSize)
{
    boardSize -= viewSize;
    position -= ci::Vec2i(1, 1);
    viewSize -= ci::Vec2i(2, 2);
    //return position - position % viewSize;
    return min(boardSize,
               position - ci::Vec2i(position.x % viewSize.x,
                                    position.y % viewSize.y));
}

void Drawer::draw(const Board &terrain, const Actor &actor) const
{
    // integer scale to preserve pixels
    const int scalei = reduce_min(mWindowSize/mTileSize/mViewSize);
    if(!scalei)
    {
        return;
    }
    const float scale = static_cast<float>(scalei);

    // actual view size (depending on window ratio)
    ci::Vec2i viewSize(mViewSize, mViewSize);
    if(mWindowSize.x>mWindowSize.y)
    {
        viewSize.x = (viewSize.x * mWindowSize.x)/mWindowSize.y;
    }
    else
    {
        viewSize.y = (viewSize.y * mWindowSize.y)/mWindowSize.x;
    }
    viewSize = min(viewSize, terrain.size());

    // select a view (scroll)
    const auto scrollOffset = positionToScrollOffset(actor.logicalPosition(),
                                                     viewSize, terrain.size());
    if(mScrollOffset.isComplete()
       && (ci::Vec2f(scrollOffset)-mScrollOffset).lengthSquared() > ci::EPSILON)
    {
        const auto lastScrollOffset = positionToScrollOffset(actor.lastPosition(),
                                                     viewSize, terrain.size());
        mTimeline->apply(&mScrollOffset,
                         ci::Vec2f(lastScrollOffset), ci::Vec2f(scrollOffset),
                         reduce_max(abs(scrollOffset-lastScrollOffset)) * SCROLL_DURATION);
    }


    ci::Vec2f offset((mWindowSize-viewSize*mTileSize*scale)*0.5f);
    offset -= mScrollOffset.value() * mTileSize * scale;

    const int actorY = std::ceil(actor.animatedPosition().y);
    for(int y=0; y<=actorY && y<terrain.height(); ++y)
    {
        auto it=terrain.beginRow(y), end=terrain.endRow(y);
        for(int x=0; it != end; ++it, ++x)
        {
            drawTile(it->groundId(), ci::Vec2f(x, y), offset, scale);
            if(it->layerId())
            {
                drawTile(it->layerId(), ci::Vec2f(x, y), offset, scale);
            }
        }
    }

    drawTile(actor.tileId(), actor.animatedPosition(), offset, scale);
    
    for(int y=actorY+1; y<terrain.height(); ++y)
    {
        auto it=terrain.beginRow(y), end=terrain.endRow(y);
        for(int x=0; it != end; ++it, ++x)
        {
            drawTile(it->groundId(), ci::Vec2f(x, y), offset, scale);
            if(it->layerId())
            {
                drawTile(it->layerId(), ci::Vec2f(x, y), offset, scale);
            }
        }
    }
}

void Drawer::drawTile(int tileId, ci::Vec2f position,
                      ci::Vec2f offset, float scale) const
{
    const auto found = mIdToTile.find(tileId);
    if(found == mIdToTile.end())
    {
        return;
    }

    const Tile & tile = found->second;
    const auto imageSize = tile.image->getBounds();
    const auto src = pickTileIndex(imageSize, mTileSize, tile.height,
                                   tile.line, tile.current);
    auto dst = ci::Rectf(position+ci::Vec2f(0.f, 1.f-static_cast<float>(tile.height)/mTileSize), position+ci::Vec2f(1.f, 1.f));
    dst.scale(mTileSize*scale);
    dst.offset(offset);
    ci::gl::draw(tile.image, src, dst);
}
