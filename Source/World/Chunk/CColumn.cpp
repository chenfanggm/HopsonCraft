#include "CColumn.h"

#include <SFML/System/Clock.hpp>
#include <cmath>

#include "../../Renderer/RMaster.h"
#include "../../Util/Random.h"

#include <iostream>



namespace Chunk
{
    Column::Column(const Position& pos, Map& map)
    :   m_position      (pos)
    ,   m_p_chunkMap    (&map)
    {
        Noise::Generator m_noiseGen;
        std::vector<int32_t> heightMap(World_Constants::CH_AREA);

        int v;
        m_noiseGen.setSeed(6446);
        //m_noiseGen.setNoiseFunction({8, 80, 0.53, 200, 0});
        m_noiseGen.setNoiseFunction({10, 80, 0.5, 240, 0});

        if( pos.x < 0 || pos.y < 0)
        {
            v = 15;
        }
        else
        {
            for (int32_t x = 0; x < World_Constants::CH_SIZE; x++)
            {
                for (int32_t z = 0; z < World_Constants::CH_SIZE; z++)
                {
                    int h = m_noiseGen.getValue(x, z, pos.x, pos.y);
                    heightMap[x * World_Constants::CH_SIZE + z] = h;
                }
            }
            v = *std::max_element(heightMap.begin(), heightMap.end());
        }




        for (int32_t y = 0; y < v + 1; y++)
        {
            for (int32_t x = 0; x < World_Constants::CH_SIZE; x++)
            {
                for (int32_t z = 0; z < World_Constants::CH_SIZE; z++)
                {
                    int h = heightMap[x * World_Constants::CH_SIZE + z];

                    if (y == h)
                    {
                        y > 75?
                            setBlock({x, y, z}, Block::ID::Grass) :
                            setBlock({x, y, z}, Block::ID::Sand);
                    }


                    else if (y < h && y > h - 3 )
                    {
                        setBlock({x, y, z}, Block::ID::Dirt);
                    }
                    else if (y <= h - 3)
                    {
                        setBlock({x, y, z}, Block::ID::Stone);
                    }
                }
            }
        }
        m_flags.generated = true;
    }

    void Column::createFullMesh()
    {
        for (auto& c : m_chunklets)
        {
            c->createMesh();
        }
        m_flags.hasFullMesh = true;
    }


    void Column::setBlock(const Block::Column_Position& pos, CBlock block)
    {
        int32_t yIndex = std::ceil(pos.y / World_Constants::CH_SIZE);
        while (yIndex + 1 > m_chunkCount)
        {
            addChunklet();
        }
        Chunklet* chunk = getChunkletnc(yIndex);

        auto yPos = pos.y - World_Constants::CH_SIZE * yIndex;
        Block::Small_Position blockPosition ((int8_t)pos.x, yPos, (int8_t)pos.z);

        chunk->qSetBlock(blockPosition, block);

        if(m_flags.generated)
        {
            std::vector<Chunklet*> chunklets = chunk->setBlock(blockPosition, block);
            m_chunkletsToUpdate.insert(m_chunkletsToUpdate.end(), chunklets.begin(), chunklets.end());
        }
        else
        {
            chunk->qSetBlock(blockPosition, block);
        }

    }

    CBlock Column::getBlock(const Block::Column_Position& pos) const
    {
        if(pos.y < 0)
        {
            return Block::ID::Air;
        }
        auto chunklet = std::ceil(pos.y / World_Constants::CH_SIZE);
        auto yPos = pos.y - World_Constants::CH_SIZE * chunklet;

        if (chunklet < m_chunkCount && chunklet >= 0)
        {
            Block::Small_Position bp (pos.x, yPos, pos.z);
            return m_chunklets.at(chunklet)->qGetBlock(bp);
        }
        else
        {
            return Block::ID::Air;
        }
    }


    Chunklet* Column::getChunklet(int32_t index)
    {
        if (index > m_chunkCount - 1)
        {
            return nullptr;
        }
        else if (index < 0)
        {
            return nullptr;
        }
        return m_chunklets.at(index).get();
    }

    Chunklet* Column::getChunkletnc(int32_t index)
    {
        if (index > m_chunkCount - 1)
        {
            return nullptr;
        }
        else if (index < 0)
        {
            return nullptr;
        }
        return m_chunklets.at(index).get();
    }


    const Position& Column::getPosition () const
    {
        return m_position;
    }

    const Column::CFlags& Column::getFlags ()
    {
        return m_flags;
    }

    void Column::setDeleteFlag (bool deleteF)
    {
        m_flags.deleteMe = deleteF;
    }

    void Column::update()
    {
        for (auto& c : m_chunkletsToUpdate)
        {
            c->createMesh();
        }
        m_chunkletsToUpdate.clear();
    }

    void Column::draw(Renderer::Master& renderer)
    {
        for(auto itr = m_chunklets.begin(); itr != m_chunklets.end();)
        {
            Chunklet& chunklet = *(*itr);
            if (chunklet.getFlags().hasFaces)
            {
                if (chunklet.getFlags().hasBuffered)
                {
                    renderer.draw(chunklet);
                    ++itr;
                }
                else
                {
                    chunklet.bufferMesh();
                }
            }
            else
            {
                ++itr;
            }
        }
    }

    void Column::addChunklet()
    {
        m_chunklets.push_back(std::make_unique<Chunklet>
                              (Chunklet_Position(m_position.x,
                                                 m_chunkCount++,
                                                 m_position.y),
                               *m_p_chunkMap));
    }


}
