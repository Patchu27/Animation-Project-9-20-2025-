#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <math.h>
#include <cstring>
#include <unordered_map>

struct BetterWindow {
    sf::RenderWindow window;
    sf::View view;
    sf::Vector2u size;
    bool isFullscreen;
    sf::String title;
    sf::ContextSettings contSettings{};
    unsigned int fps;
    bool useVsync;

    void recreateWindow() {
        window.create(sf::VideoMode(size), title, static_cast<sf::State>(isFullscreen), contSettings);
        window.setVerticalSyncEnabled(useVsync);
        window.setFramerateLimit(fps);
    }

    BetterWindow(const sf::Vector2u& size_ = { 1920u,1080u }, const unsigned int fps_ = 60u, const bool useVsync_ = 0, const bool fullscreen_ = 1, const sf::String title_ = "Title", const sf::ContextSettings& contSettings_ = {}) {
        size = size_;
        fps = fps_;
        useVsync = useVsync_;
        isFullscreen = fullscreen_;
        title = title_;
        contSettings = contSettings_;
        recreateWindow();
        view.setSize(static_cast<sf::Vector2f>(window.getSize()));
        view.setCenter(view.getSize().componentWiseMul({ 0.5f,0.5f }));
        window.setView(view);
    }

    void toggleFullscreen() {
        isFullscreen = !isFullscreen;
        recreateWindow();
    }
};

namespace BetterInput {
    enum class Key {
        Unknown = -1,
        A = 0, B, C,
        D, E, F, G,
        H, I, J, K,
        L, M, N, O,
        P, Q, R, S,
        T, U, V, W,
        X, Y, Z,
        Num0,
        Num1, Num2, Num3, Num4,
        Num5, Num6, Num7, Num8,
        Num9, Escape, LControl, LShift,
        LAlt, LSystem, RControl, RShift,
        RAlt, RSystem, Menu, LBracket,
        RBracket, Semicolon, Comma, Period,
        Apostrophe, Slash, Backslash, Grave,
        Equal, Hyphen, Space, Enter,
        Backspace, Tab, PageUp, PageDown,
        End, Home, Insert, Delete,
        Add, Subtract, Multiply, Divide,
        Left, Right, Up, Down,
        Numpad0, Numpad1, Numpad2, Numpad3,
        Numpad4, Numpad5, Numpad6, Numpad7,
        Numpad8, Numpad9, F1, F2,
        F3, F4, F5, F6,
        F7, F8, F9, F10,
        F11, F12, F13, F14,
        F15, Pause,

        MouseLeft, MouseRight, MouseMiddle, MouseExtra1,
        MouseExtra2
    };
    //to add mouse buttons, utilize the existing system but replace the SFML enums with your own to unify the keyboard keys and mouse buttons into a unified input stream
    bool keyBuffer[106] = {};//101+5
    bool keyHeld[106] = {};
    std::vector<BetterInput::Key> keyUpdateList;

    static void setUpdateList(const std::initializer_list<BetterInput::Key>& updateList_ = {}) {
        keyUpdateList = updateList_;
    }

    static void updateKeyList() {
        size_t listSize = keyUpdateList.size();
        for (size_t i = 0; i < listSize; i++) {
            const std::uint8_t keyPos = static_cast<std::uint8_t>(keyUpdateList[i]);
            keyBuffer[keyPos] = keyHeld[keyPos];
            if (keyPos < 100)
                keyHeld[keyPos] = sf::Keyboard::isKeyPressed(static_cast<sf::Keyboard::Key>(keyPos));
            else
                keyHeld[keyPos] = sf::Mouse::isButtonPressed(static_cast<sf::Mouse::Button>(keyPos - 101));
        }
    }

    static bool keyPressed(const BetterInput::Key& key_, bool hold_ = 1) {
        //It's important to note that if you check a key conditionally (ex. if (Akey && *B*key) do something) 
        //-> then the polling for B key will fail because it's buffer isn't updated every frame.
        //To solve this, either make your own name/buffer for every key you need (ex. r_key=BetterInput::keyPress(sf::Keyboard::Key::D)), 
        //-> or make temporary buffers in the scope you're executing something to avoid checking+updating keys sequentially and conditionally.
        //New solution, I'm adding a constant update list that will be largely managed by the namespace
        //Newer solution, force syncronized global updates, and poll a copy of the result from an array
        if (key_ != BetterInput::Key::Unknown) {
            if (keyHeld[static_cast<std::uint8_t>(key_)]) {
                bool returnVal = 0;

                if (hold_) {
                    returnVal = 1;
                }
                else {
                    if (!keyBuffer[static_cast<std::uint8_t>(key_)])
                    {
                        // std::cout << "pressed";
                        returnVal = 1;
                    }
                    else {
                        //  std::cout << "noRepeat";
                        returnVal = 0;
                    }
                }
                return returnVal;
            }
            else {
                return 0;
            }

        }
        else {
            return 0;
        }

    }

    static bool keyReleased(const BetterInput::Key& key_) {
        //It's important to note that if you check a key conditionally (ex. if (Akey && *B*key) do something) 
        //-> then the polling for B key will fail because it's buffer isn't updated every frame.
        //To solve this, either make your own name/buffer for every key you need (ex. r_key=BetterInput::keyPress(sf::Keyboard::Key::D)), 
        //-> or make temporary buffers in the scope you're executing something to avoid checking+updating keys sequentially and conditionally.
        //New solution, I'm adding a constant update list that will be largely managed by the namespace
        //Newer solution, force syncronized global updates, and poll a copy of the result from an array
        if (key_ != BetterInput::Key::Unknown) {
            if ((!keyHeld[static_cast<std::uint8_t>(key_)]) && (keyBuffer[static_cast<std::uint8_t>(key_)]))
                return 1;
            else
                return 0;

        }
    }

    static sf::Vector2i getCursorPosition(const sf::RenderWindow& window) {
        return sf::Mouse::getPosition(window);
    }
    static sf::Vector2i getCursorPosition() {
        return sf::Mouse::getPosition();
    }

}

namespace BetterSound {

    struct soundInfo {
        float volume = 1.f;
        float pitch = 1.f;
        soundInfo(const float volume_ = 0.f, const float pitch_ = 1.f) {
            volume = volume_;
            pitch = pitch_;
        }
    };

    static inline std::vector<sf::SoundBuffer> loadedSounds;
    static inline std::unordered_map<std::string, size_t> soundNameTable;

    static inline sf::SoundBuffer defSoundBuf{};

    static inline std::vector<sf::Sound> soundChannels{ 256, sf::Sound{defSoundBuf} };//*if you ever have an audio problem check the fact that this is 256 and not 255 size
    static inline std::uint8_t nextAvailableChannel = 0;
    bool channelsBusy = 0;

    static void refreshChannelAvailability() {//refreshes availability status
        std::uint8_t nACCopy = nextAvailableChannel;
        for (std::uint8_t i = 1; i != 0; i++) {//utilizes rollover
            nACCopy++;
            if (soundChannels[nACCopy].getStatus() != sf::Sound::Status::Playing) {
                channelsBusy = 0;
                nextAvailableChannel = nACCopy - 1;
                break;
            }
        }
    }

    static void playSound(const std::string& soundName_, const float volume_ = 1.f, const float pitch_ = 1.f) {

        if (soundNameTable.contains(soundName_)) {
            if (!channelsBusy) {

                bool foundAvailable = 0;
                for (std::uint8_t i = 1; i != 0; i++) {//utilizes rollover
                    nextAvailableChannel++;
                    if (soundChannels[nextAvailableChannel].getStatus() != sf::Sound::Status::Playing) {
                        foundAvailable = 1;
                        break;
                    }
                }
                if (foundAvailable) {
                    soundChannels[nextAvailableChannel].setVolume(volume_ * 100.f);
                    soundChannels[nextAvailableChannel].setPitch(pitch_);//this sets the last set pitch since the last refresh, a compromise. Volume can be additive, but not pitch since it changes length. Unlikely scenario to overlap sounds, and so this only helps if you're already about to potentially break your sound.. well if you put even 2 it breaks... hm
                    soundChannels[nextAvailableChannel].setBuffer(loadedSounds[soundNameTable[soundName_]]);
                    soundChannels[nextAvailableChannel].play();//if two sounds are played with a significant amount of computation in between, 
                }
                else {
                    channelsBusy = 1;
                }
            }
            //add functionality for playing only a selected length of the sound file (for combined files)
        }
        else {
            std::cout << "BETTER SOUND: attempted to play an unloaded sound\n";
        }

    }

    static void playQueuedSounds() {//play from buffer
                
            
        //soundInfo info = { 0.f,1.f };
        //std::fill(queuedSoundsBuffer.begin(), queuedSoundsBuffer.end(), *info);//reset buffer volumes/queueing
        //this refreshes channelsBusy once sounds are done being set to channels for the current frame
        refreshChannelAvailability();
    }

    static void loadSound(const std::string& path_) {
        const std::uint16_t pathLength = static_cast<std::uint16_t>(path_.length());
        if (pathLength) {
            std::string fileName;
            for (size_t j = pathLength - 1; j < pathLength; j--) {
                if ((path_.at(j) == '/') || (j == 0)) {
                    if (j) j++;//if condition corrects for pathless string cutting off the first symbol
                    fileName = path_.substr(j, pathLength - j);
                    break;
                }
            }
            loadedSounds.emplace_back();
            if (loadedSounds[loadedSounds.size() - 1].loadFromFile(path_)) {
                soundNameTable[fileName] = loadedSounds.size() - 1;
            }
            else {
                loadedSounds.resize(loadedSounds.size() - 1);
                std::cout << "BETTER SOUND: Sound file not found or failed to load\n";
            }
        }
        else {
            std::cout << "BETTER SOUND: No path provided\n";
        }
    }

    static void batchLoadSound(const std::initializer_list<std::string>& filePaths_) {
        const size_t listSize = filePaths_.size();
        loadedSounds.reserve(listSize);
        for (size_t i = 0; i < listSize; i++) {
            loadSound(filePaths_.begin()[i].data());
        }
    }

}

class BetterSprites {
public:


    struct TextureInfo {

        std::uint16_t tileCount;
        sf::Vector2f tileSize;
        std::string path;

        TextureInfo(const sf::Vector2f& tileSize_, const std::uint16_t& tileCount_, const std::string& path_) {
            tileSize = tileSize_;
            tileCount = tileCount_;
            path = path_;
        }
    };

    struct TextureContainer {
        sf::Texture texture;
        sf::Vector2f textureSize;
        sf::Vector2f tileSize;//self auto initialized
        std::uint16_t rowlength = 0;
        std::uint16_t totalTiles = 0;
        TextureContainer() {

        }


        bool loadTexture(const BetterSprites::TextureInfo& texInfo_) {
            if (texture.loadFromFile(texInfo_.path)) {
                textureSize = static_cast<sf::Vector2f>(texture.getSize());
                tileSize = texInfo_.tileSize;
                rowlength = static_cast<std::uint16_t>(textureSize.x / tileSize.x);//should truncate
                totalTiles = texInfo_.tileCount;//cannot be determined by area because the animation/tilesheet might not fill the whole image
                return 1;
            }
            else {
                return 0;
            }
        }

    };

    struct VertexContainer {
        std::vector<std::uint32_t> inactiveIndices;
        std::vector<std::uint32_t> activeIndices;
        sf::VertexArray vertices;
        std::uint32_t textureIndex;
        std::vector<std::uint16_t> tileIndices;
        std::vector<float> angles;
        std::vector<sf::Vector2f> scales;
        std::vector<sf::Vector2f> offsets;
        std::vector<sf::Vector2f> positions;

        size_t deleteCount;

        VertexContainer(const std::uint32_t& textureIndex_) {
            vertices.setPrimitiveType(sf::PrimitiveType::Triangles);
            deleteCount = 0;
            textureIndex = textureIndex_;
        }


        void updateVertices(const std::uint32_t& spriteIndex_, const sf::Vector2f& position_, const sf::Vector2f& scale_, const std::uint16_t& imageIndex_) {
            // const std::uint32_t readActive = activeIndices[spriteIndex_];//read from the end of the end of activeIndices. It's an index array. It often doesn't line up with the vertices directly. This is also necessary for when updating a preexisting entry
            const size_t vertIndex = activeIndices[spriteIndex_] * 6;
            const sf::Vector2f texSize = static_cast<sf::Vector2f>(loadedTextures[textureIndex].tileSize);//previously textureSize by mistake

            const sf::Vector2f realSize = texSize.componentWiseMul(scale_);

            //offsets[readActive] = realSize.componentWiseMul({-0.5f,-0.5f});


            sf::Vector2f posDelta = -offsets[spriteIndex_];
            

            vertices[vertIndex].position = position_ + posDelta;
            vertices[vertIndex + 1].position = position_ + realSize + posDelta;
            vertices[vertIndex + 2].position = sf::Vector2f{ position_.x, position_.y + realSize.y } + posDelta;//more efficient ig, skips extra vector2f contruction//position_ + realSize.componentWiseMul({ 0.f,1.f }) + posDelta;
            vertices[vertIndex + 3].position = position_ + posDelta;
            vertices[vertIndex + 4].position = sf::Vector2f{ position_.x + realSize.y, position_.y } + posDelta;
            vertices[vertIndex + 5].position = position_ + realSize + posDelta;

            const sf::Color col{ 255,255,255 };

            vertices[vertIndex].color = col;
            vertices[vertIndex+1].color = col;
            vertices[vertIndex+2].color = col;
            vertices[vertIndex+3].color = col;
            vertices[vertIndex+4].color = col;
            vertices[vertIndex+5].color = col;

            updateTileIndex(spriteIndex_, imageIndex_);

        }
        void updatePosition(const std::uint32_t& spriteIndex_, const sf::Vector2f& position_, const bool additive_ = 0) {
            // const std::uint32_t readActive = activeIndices[spriteIndex_];//read from the end of the end of activeIndices. It's an index array. It often doesn't line up with the vertices directly. This is also necessary for when updating a preexisting entry
            const size_t vertIndex = activeIndices[spriteIndex_] * 6;

            sf::Vector2f posDelta{};

            if (additive_){
            posDelta = position_;
                positions[spriteIndex_] += position_;
        }
            else {
                posDelta = position_ - positions[spriteIndex_];
                positions[spriteIndex_] = position_;
            }

              //  posDelta += (vertices[vertIndex + 1].position - vertices[vertIndex].position).componentWiseMul({ -0.5f,-0.5f });
            //posDelta -= offsets[spriteIndex_];

            vertices[vertIndex].position += posDelta;
            vertices[vertIndex + 1].position += posDelta;
            vertices[vertIndex + 2].position += posDelta;
            vertices[vertIndex + 3].position += posDelta;
            vertices[vertIndex + 4].position += posDelta;
            vertices[vertIndex + 5].position += posDelta;
        }

        sf::Vector2f rotate(const sf::Vector2f& currentPos_, float angle_) {
            // Translate point to origin
            float dx = currentPos_.x;
            float dy = currentPos_.y;

            // Apply rotation
            float x_rot =  dy * sin(angle_) - dx * cos(angle_);
            float y_rot = dx * sin(angle_) + dy * cos(angle_);

            // Translate back
            return { x_rot, y_rot };
        }


        void updateRotation(const std::uint32_t& spriteIndex_, const float& angle_, const bool centered_ = 1, const bool additive_ = 0) {
            const size_t vertIndex = activeIndices[spriteIndex_] * 6;
            sf::Vector2f anchorPos = {};// -offsets[spriteIndex_];

               const float rotation = angle_ + angles[spriteIndex_] * additive_;//additive bool
                

            angles[spriteIndex_] = rotation;

            //  if (centered_) 
           // anchorPos += sf::Vector2f(vertices[vertIndex + 1].position - vertices[vertIndex].position).componentWiseMul({ 0.5f,0.5f });
            //scales from center
            const sf::Vector2f scalar = scales[spriteIndex_].componentWiseMul(loadedTextures[textureIndex].tileSize).componentWiseMul({ 0.70710678f,0.70710678f });
            //sf::Vector2f scalar = scales[spriteIndex_].componentWiseMul(loadedTextures[textureIndex].tileSize);

            const sf::Vector2f pos = positions[spriteIndex_] + sf::Vector2f{ sin(rotation),cos(rotation)}.componentWiseMul(offsets[spriteIndex_]- scales[spriteIndex_].componentWiseMul(loadedTextures[textureIndex].tileSize).componentWiseMul({ 0.5f,0.5f }));

  
            vertices[vertIndex + 0].position = rotate(sf::Vector2f(sin(2.35619449f) * scalar.x, cos(2.35619449f) * scalar.y), rotation) + pos;
                vertices[vertIndex + 1].position = rotate(sf::Vector2f(sin(5.49778715f) * scalar.x, cos(5.49778715f) * scalar.y), rotation) + pos;
                vertices[vertIndex + 2].position = rotate(sf::Vector2f(sin(3.92699082f) * scalar.x, cos(3.92699082f) * scalar.y), rotation) + pos;

                vertices[vertIndex + 3].position = rotate(sf::Vector2f(sin(2.35619449f) * scalar.x, cos(2.35619449f) * scalar.y), rotation) + pos;
                vertices[vertIndex + 4].position = rotate(sf::Vector2f(sin(0.78539816f) * scalar.x, cos(0.78539816f) * scalar.y), rotation) + pos;
                vertices[vertIndex + 5].position = rotate(sf::Vector2f(sin(5.49778715f) * scalar.x, cos(5.49778715f) * scalar.y), rotation) + pos;
            

        }

        void updateTileIndex(const std::uint32_t& spriteIndex_, const std::uint16_t& tileIndex_, bool additive = 0) {
            // const std::uint32_t readActive = activeIndices[spriteIndex_];//read from the end of the end of activeIndices. It's an index array. It often doesn't line up with the vertices directly. This is also necessary for when updating a preexisting entry
            std::uint16_t tempTileIndex = tileIndices[spriteIndex_];
            tempTileIndex = (tileIndex_ + additive * tempTileIndex) % loadedTextures[textureIndex].totalTiles;
            tileIndices[spriteIndex_] = tempTileIndex;

            const size_t vertIndex = activeIndices[spriteIndex_] * 6;

            const sf::Vector2f tileSize = loadedTextures[textureIndex].tileSize;

            const std::uint16_t tY = tempTileIndex / loadedTextures[textureIndex].rowlength;//after refactoring (switching to function access with "."), there won't be any need to use containerIndex. Access will be handled directly using 1 index.
            const std::uint16_t tX = tempTileIndex - (tY * loadedTextures[textureIndex].rowlength);
            sf::Vector2f texPos = sf::Vector2f{ static_cast<float>(tX),static_cast<float>(tY) }.componentWiseMul(tileSize);

            vertices[vertIndex].texCoords = texPos;
            vertices[vertIndex + 1].texCoords = texPos + tileSize;
            vertices[vertIndex + 2].texCoords = sf::Vector2f{ texPos.x, texPos.y + tileSize.y };
            vertices[vertIndex + 3].texCoords = texPos;
            vertices[vertIndex + 4].texCoords = sf::Vector2f{ texPos.x + tileSize.x, texPos.y };//slower//texPos + tileSize.componentWiseMul({ 1.f,0.f });//vertices[vertIndex + 2].texCoords;
            vertices[vertIndex + 5].texCoords = texPos + tileSize;
        }

        void updateColor(const std::uint32_t& spriteIndex_, const sf::Color& color_) {
            const size_t vertIndex = activeIndices[spriteIndex_] * 6;
            vertices[vertIndex].color = color_;
            vertices[vertIndex+1].color = color_;
            vertices[vertIndex+2].color = color_;
            vertices[vertIndex+3].color = color_;
            vertices[vertIndex+4].color = color_;
            vertices[vertIndex+5].color = color_;
        }

        sf::Color getColor(const std::uint32_t& spriteIndex_) {
            return vertices[activeIndices[spriteIndex_] * 6].color;//just poll the origin vertex
        }

        void addSprite(const sf::Vector2f& position_, const sf::Vector2f& scale_, const bool centered_, const std::uint16_t& imageIndex_ = 0,const float& rotation_ = 0.f) {
            const std::uint32_t listSize = static_cast<std::uint32_t>(inactiveIndices.size());
            if (listSize > 0) {
                activeIndices.emplace_back(inactiveIndices[listSize - 1]);
                //tileIndices does not rely on old information, therefore it doesn't need to be appended conditionally
                inactiveIndices.resize(listSize - 1);
            }
            else {
                activeIndices.emplace_back(activeIndices.size());
                //tilesIndices moved down outside of condition

                for (std::uint8_t n = 0; n < 6; n++) {
                    vertices.append({});
                }

                // offsets.emplace_back(sf::Vector2f{0.f,0.f});
            }

            tileIndices.emplace_back(imageIndex_);
            angles.emplace_back(rotation_);
            scales.emplace_back(scale_);
            positions.emplace_back(position_);

            if (centered_)
                offsets.emplace_back(sf::Vector2f{ loadedTextures[textureIndex].tileSize.componentWiseMul({0.5f*scale_.x,0.5f*scale_.y}) });
            else
                offsets.emplace_back(sf::Vector2f{0.f,0.f});
            
           //this was here before, and works somehow but seems incorrect
           //updateVertices(static_cast<std::uint32_t>(activeIndices.size() - 1), position_, scale_, centered_);
            //keep an eye out for problems here
            //WHY DOES THIS WORK?
            //it works because the index is figured in the function and the size is equal to the index in this case
            updateVertices(static_cast<std::uint32_t>(activeIndices.size() - 1), position_, scale_,imageIndex_);
            updateRotation(static_cast<std::uint32_t>(activeIndices.size() - 1), rotation_, 0);//look over for review
        }

        void minimizeVertices(const std::uint32_t& spriteIndex_) {
            deleteCount++;
            const size_t vertIndex = activeIndices[spriteIndex_] * 6;
            for (std::uint8_t i = 0; i < 6; i++) {
                vertices[vertIndex + i].position = {};
            }
        }

        void deleteSprite(const std::uint32_t& spriteIndex_) {
            if (spriteIndex_ < activeIndices.size()) {
                minimizeVertices(spriteIndex_);
                inactiveIndices.emplace_back(activeIndices[spriteIndex_]);
                if (spriteIndex_ != activeIndices.size() - 1) {
                    std::memmove(&activeIndices[spriteIndex_], &activeIndices[spriteIndex_ + 1], sizeof(std::uint32_t) * (activeIndices.size() - spriteIndex_));
                    std::memmove(&tileIndices[spriteIndex_], &tileIndices[spriteIndex_ + 1], sizeof(std::uint16_t) * (tileIndices.size() - spriteIndex_));
                    std::memmove(&angles[spriteIndex_], &angles[spriteIndex_ + 1], sizeof(float) * (angles.size() - spriteIndex_));
                    std::memmove(&scales[spriteIndex_], &scales[spriteIndex_ + 1], sizeof(sf::Vector2f) * (scales.size() - spriteIndex_));
                    std::memmove(&offsets[spriteIndex_], &offsets[spriteIndex_ + 1], sizeof(sf::Vector2f) * (offsets.size() - spriteIndex_));
                    std::memmove(&positions[spriteIndex_], &positions[spriteIndex_ + 1], sizeof(sf::Vector2f) * (positions.size() - spriteIndex_));
                }
                
                //Make a struct called SpriteInfo to store all of these in one object. It would cut down on code and memmove calls
                activeIndices.resize(activeIndices.size() - 1);
                tileIndices.resize(tileIndices.size() - 1);
                angles.resize(angles.size() - 1);
                scales.resize(scales.size() - 1);
                offsets.resize(offsets.size() - 1);
                positions.resize(positions.size() - 1);
                //add subsequent functions here
            }
            else {
                std::cout << "BETTER SPRITES: attempted to delete a sprite at an invalid index\n";
            }
        }

        sf::Vector2f getPosition(const std::uint32_t& spriteIndex_) {
            const std::uint32_t vertIndex = activeIndices[spriteIndex_]*6;
            return positions[spriteIndex_];
        }

        float getRotation(const std::uint32_t& spriteIndex_) {
            return angles[spriteIndex_];
        }

        std::uint16_t getTileIndex(const std::uint32_t& spriteIndex_) {
            return tileIndices[spriteIndex_];
        }

        size_t getSpriteCount() {
            return activeIndices.size();
        }

    };

    static inline std::unordered_map<std::string, std::uint16_t> textureNameTable;
    static inline std::vector <BetterSprites::TextureContainer> loadedTextures;
    typedef BetterSprites::VertexContainer& ContainerReference;

    static std::uint16_t textureLookup(const std::string& textureName_) {//returns the integer ID of the provided texture
        if (textureNameTable.contains(textureName_)) {
            return textureNameTable[textureName_];
        }
        else {
            std::cout << "BETTER SPRITES: texture not indexed\n";
            return 0;//to satisfy return expectation
        }
    }

    static void loadTexture(const BetterSprites::TextureInfo& texInfo_) {
        const std::uint16_t pathLength = static_cast<std::uint16_t>(texInfo_.path.length());
        if (pathLength) {
            std::string fileName;
            for (size_t j = pathLength - 1; j < pathLength; j--) {
                if ((texInfo_.path.at(j) == '/') || (j == 0)) {
                    if (j) j++;//if condition corrects for pathless string cutting off the first symbol
                    fileName = texInfo_.path.substr(j, pathLength - j);
                    break;
                }
            }

            loadedTextures.emplace_back();
            if (loadedTextures[loadedTextures.size() - 1].loadTexture(texInfo_)) {
                //BetterSprites::addContainer(textureNameTable.size());
                textureNameTable[fileName] = static_cast<std::uint16_t>(loadedTextures.size() - 1);
            }
            else {
                loadedTextures.resize(loadedTextures.size() - 1);
                std::cout << "BETTER SPRITES: Image file not found or failed to load\n";
            }
        }
        else {
            std::cout << "BETTER SPRITES: No path provided\n";
        }
    }

    static void batchLoadTexture(const std::initializer_list<BetterSprites::TextureInfo>& texInfoList_) {
        const size_t listSize = texInfoList_.size();
        loadedTextures.reserve(listSize);
        for (size_t i = 0; i < listSize; i++) {
            loadTexture(texInfoList_.begin()[i]);
        }
    }

    class Layer {
        //private:
    public:
        std::vector<BetterSprites::VertexContainer> uniqueVAList;
        std::vector<std::uint16_t> vaDepthMap;

        void addContainer(const std::uint16_t& textureIndex_) {
            // if (textureIndex_ >= uniqueVAList.size()) {

            uniqueVAList.emplace_back(textureIndex_);
            //  }
        }

        Layer() {

        }

        

        void drawSprites(sf::RenderTarget& target_, bool drawAll_, std::uint16_t arrayIndex_ = 0) {
            std::uint16_t endIndex;
            if (drawAll_) endIndex = static_cast<std::uint16_t>(uniqueVAList.size());
            else
                endIndex = arrayIndex_ + 1;

          //  size_t listSize = uniqueVAList.size();
            for (; arrayIndex_ < endIndex; arrayIndex_++) {
                target_.draw(uniqueVAList[arrayIndex_].vertices, &loadedTextures[uniqueVAList[arrayIndex_].textureIndex].texture);//do not use VertexBuffer
            }
        }

        size_t getSpriteCount(const std::uint16_t& textureIndex) {
            if (textureIndex < uniqueVAList.size()) {
                return uniqueVAList[textureIndex].activeIndices.size();
            }
            else {
                std::cout << "BETTER SPRITES: attempted get the sprite count of a vertex array with an unloaded texture\n";
                return 0;//to satisfy return expectation
            }
        }


        BetterSprites::VertexContainer& getContainer(const std::uint16_t& textureIndex_) {
            if (textureIndex_ < uniqueVAList.size()) {
                return uniqueVAList[textureIndex_];
            }
            else {
                std::cout << "Texture index does not exist\n";
                throw std::invalid_argument("Texture index does not exist\n");
            }
        }
    };
};

namespace BetterUtility {
    sf::Vector2f randVec2(const sf::Vector2u& bounds_) {
        return static_cast<sf::Vector2f>(sf::Vector2u{ rand() % bounds_.x, rand() % bounds_.y });
    }

   float clamp(const float& input_,const float& min_, const float max_) {
       if (input_ < min_) return min_;
       else
           if (input_ > max_) return max_;
          
               return input_;
    }
   int clamp(const int& input_, const int& min_, const int max_) {
       if (input_ < min_) return min_;
       else
           if (input_ > max_) return max_;

       return input_;
   }
   unsigned int clamp(const unsigned int& input_, const unsigned int& min_, const unsigned int max_) {
       if (input_ < min_) return min_;
       else
           if (input_ > max_) return max_;

       return input_;
   }

    std::int8_t sign(float input) {
        return (input > 0) - (input < 0);
    }
    std::int8_t sign(double input) {
        return (input > 0) - (input < 0);
    }
    std::int8_t sign(int input) {
        return (input > 0) - (input < 0);
    }

}



std::string floatToString(float value, int precision) {
    // Extract integer part using type cast (bitwise safe)
    int intPart = static_cast<int>(value);

    // Get fractional part
    float fracPart = value - intPart;

    // Scale fractional part
    int scale = static_cast<int>(std::pow(10, precision));
    int scaledFrac = static_cast<int>(fracPart * scale);

    // Bitwise formatting
    std::string fracStr = std::to_string(scaledFrac);
    std::string result = std::to_string(intPart) + "." + fracStr;

    // Pad fractional part with leading zeros if needed
  //  while (fracStr.length() < static_cast<size_t>(precision)) {
  //      fracStr = "0" + fracStr;
 //   }
    return result;
}

int main()
{
    sf::Font sansSerifFont;
    sansSerifFont.openFromFile("./Fonts/archistico/Archistico_Simple.ttf");
   // sf::Text penRadiusText(sansSerifFont);//old name
    sf::Text notificationText(sansSerifFont);
    notificationText.setOutlineColor({ 0,0,0,255 });
    notificationText.setOutlineThickness(-2.f);
    //notificationText.setString("TEST");
    std::uint16_t notificationTextTimer = 0;
    std::uint16_t notificationTextTimerStart = 120;//this will be doubled

    std::vector<sf::Image> undoVector;

    float penRadius = 10.f; //modifiable
    float penDiameter = penRadius * 2.f;

    sf::Vector2f lastPenPoint{};

    // sf::Image cursorImage;
    //sf::Texture cursorTexture;
    sf::Color penColor = sf::Color{ 0,0,0,255 };
    sf::CircleShape cursorCircle(penRadius, static_cast<size_t>(ceil(penRadius * 10.f)));//5.f is probably fine//scales with circumference
    cursorCircle.setFillColor(penColor);

    sf::RenderTexture cursorRenderTexture;
    cursorRenderTexture.resize({static_cast<std::uint32_t>(penDiameter+1), static_cast<std::uint32_t>(penDiameter+1)});//+1 because otherwise it gets cut off//resizes dimensions to the size of the diameter of the circle/cursor
    cursorRenderTexture.draw(cursorCircle);
    cursorRenderTexture.display();

    sf::Texture cursorTexture{ cursorRenderTexture.getTexture() };
    sf::Sprite cursorSprite(cursorTexture);//this saves the output circle as a texture so the vertices don't have to be drawn every time
    cursorSprite.setOrigin({penRadius,penRadius});

    //when done with pen stroke vv, download output texture from render texture into an image, and file the image into the undoVector
    sf::Image cursorImage{ cursorTexture.copyToImage() };

    sf::Cursor cursor(cursorImage.getPixelsPtr(), cursorImage.getSize(), cursorImage.getSize().componentWiseDiv({ 2,2 }));

    //Update Key list (keys must be included to be polled!)
    BetterInput::setUpdateList({ BetterInput::Key::MouseMiddle,BetterInput::Key::Tab,BetterInput::Key::Escape,BetterInput::Key::MouseRight,BetterInput::Key::MouseLeft, BetterInput::Key::LControl,BetterInput::Key::LShift,BetterInput::Key::B});

    //Window creation using BetterWindow constructor
    //1920x1080p, 60fps, Vsync, Fullscreen
    BetterWindow bw(sf::Vector2u{ 1920u,1080u }, 65, 1, 1);//0, 1, 1 --> no frame limit, VSync enabled, Fullscreen enabled
    //bw.window.setMouseCursor(cursor);
    bw.window.setMouseCursorVisible(0);

    sf::RenderTexture canvasRendTex(bw.size);
    sf::RenderTexture tempDrawRendTex(canvasRendTex.getSize());
    sf::Sprite tempDrawSprite(tempDrawRendTex.getTexture());
    //canvasRendTex.clear(sf::Color(128,128,128));
    canvasRendTex.clear(sf::Color(255,255,255));
    tempDrawRendTex.clear(sf::Color(0,0,0,0));
    sf::Sprite canvasRendSprite(canvasRendTex.getTexture());

    sf::RenderTexture worldRendTex(bw.size);//this is replacing the window to reserve window drawing to gui/hud components. Fixes draw order as well
    worldRendTex.clear();
    sf::Sprite worldRendSprite(worldRendTex.getTexture());

    sf::View worldView;
    worldView.setSize(static_cast<sf::Vector2f>(bw.size));
 
    sf::Vector2f mousePos;
    bool panCam = 0;
    float mouseScrollDelta = 0.f;
    float zoomDelta = 0.f;
    float zoomFactor = 1.f;

    sf::Vector2f defaultViewSize{};
    defaultViewSize = static_cast<sf::Vector2f>(sf::VideoMode::getDesktopMode().size);//Gets desktop resolution

    sf::Vector2f mouseDelt{};

    //Start the game loop
    while (bw.window.isOpen())
    {

        //Update inputs
        BetterInput::updateKeyList();



        if (BetterInput::keyPressed(BetterInput::Key::Tab, 0))
            bw.toggleFullscreen();

        if (BetterInput::keyPressed(BetterInput::Key::Escape, 0))
            bw.window.close();



        //Handle manual window close
        while (const std::optional event = bw.window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                bw.window.close();
            if (const auto* scroll = event->getIf<sf::Event::MouseWheelScrolled>()) {
                mouseScrollDelta = -scroll->delta;
                std::cout << "Wheel delta: " << scroll->delta << std::endl;
                if (BetterInput::keyPressed(BetterInput::Key::LControl, 1)) {
                    zoomDelta = (mouseScrollDelta) * 0.01f;
                    //worldView.zoom(zoomDelta+1.f);//this accumulates rounding errors
                    //zoomFactor += zoomDelta;
                    zoomFactor = zoomFactor * (1.f+zoomDelta);
                    if (zoomFactor < 0.01f) zoomFactor = 0.01f;
                    if (zoomFactor > 10.f) zoomFactor = 10.f;
                    //worldView.setSize(static_cast<sf::Vector2f>(bw.window.getSize()).componentWiseMul({zoomFactor,zoomFactor}));//this does not accumulate rounding errors
                    worldView.setSize(defaultViewSize.componentWiseMul({zoomFactor,zoomFactor}));//this does not accumulate rounding errors
                }

                if (BetterInput::keyPressed(BetterInput::Key::LShift, 1)) {
                   // penRadius += mouseScrollDelta * 0.1f;
                    if (mouseScrollDelta > 0)
                        penRadius += 0.1f;//slightly less rounding errors
                    else
                        penRadius -= 0.1f;
                    if (penRadius < 0.25f) penRadius = 0.25f;
                    penDiameter = penRadius * 2.f;

                    cursorCircle.setRadius(penRadius);
                    cursorCircle.setPointCount(static_cast<size_t>(ceil(penRadius * 10.f)));//5.f is probably fine

                    cursorRenderTexture.resize({ static_cast<std::uint32_t>(penDiameter+1), static_cast<std::uint32_t>(penDiameter+1) });//resizes dimensions to the size of the diameter of the circle/cursor
cursorRenderTexture.clear(sf::Color(0,0,0,0));//ADDED IN LINUX PORT (Mar. 23 2026)
                    cursorRenderTexture.draw(cursorCircle);
                    cursorRenderTexture.display();

                    cursorTexture = cursorRenderTexture.getTexture();
                    cursorSprite.setTexture(cursorTexture,1);//this saves the output circle as a texture so the vertices don't have to be drawn every time
                    cursorSprite.setOrigin({ penRadius,penRadius });

                    
                    //penRadiusText.setString("Pen radius set: " + std::stoi(std::to_string(penRadius)));
                   // penRadiusText.setString("Pen radius set: " + std::to_string(penRadius));
                    notificationText.setString("Pen radius set: " + floatToString(penRadius,2));
                    notificationText.setPosition(static_cast<sf::Vector2f>(bw.window.getSize()).componentWiseMul({0.5f,0.5f}) - notificationText.getLocalBounds().size.componentWiseMul({ 0.5f,0.5f }));

                    notificationTextTimer = notificationTextTimerStart;

                    notificationText.setFillColor({ 255,255,255,255 });
                }

            }
            if (const auto* mouseMove = event->getIf<sf::Event::MouseMovedRaw>()) {
                    mouseDelt = static_cast<sf::Vector2f>(mouseMove->delta).componentWiseMul({-zoomFactor,-zoomFactor});//- bc you're pushing
                    //mousePos = (static_cast<sf::Vector2f>(BetterInput::getCursorPosition(bw.window)).componentWiseDiv(static_cast<sf::Vector2f>(bw.window.getSize()))).componentWiseMul(worldView.getSize()) + worldView.getCenter() - worldView.getSize().componentWiseMul({ 0.5f,0.5f });
                    if (BetterInput::keyPressed(BetterInput::Key::MouseMiddle, 1))
                        worldView.move(mouseDelt);
                    //bw.view.move(mouseDelt);//WIP LEFT OFF HERE 3:30am
            }
        }

        //Clear, refresh channel availability
        BetterSound::refreshChannelAvailability();

        bw.window.clear();
        worldRendTex.clear({ 160,160,210,255 });
        //this is only used for canvas / window polling, repoll for drawing
        //mousePos = (static_cast<sf::Vector2f>(BetterInput::getCursorPosition(bw.window)) + bw.view.getCenter() - bw.view.getSize().componentWiseMul({ 0.5f,0.5f }));
        //mousePos = static_cast<sf::Vector2f>(BetterInput::getCursorPosition(bw.window)) + worldView.getCenter() - worldView.getSize().componentWiseMul({0.5f,0.5f});
        //FINALLY
        mousePos = (static_cast<sf::Vector2f>(BetterInput::getCursorPosition(bw.window)).componentWiseDiv(static_cast<sf::Vector2f>(bw.window.getSize()))).componentWiseMul(worldView.getSize()) + worldView.getCenter() - worldView.getSize().componentWiseMul({0.5f,0.5f});
         
        if (canvasRendTex.getViewport(bw.view).contains(static_cast<sf::Vector2i>(mousePos))) {
            bw.window.setMouseCursorVisible(0);
            canvasRendTex.clear({ 255,255,255,255 });
            //render     
            
            //poll the mouse as late as possible for visual purposes and store the position for the rest of the frame
            if (0) mousePos = static_cast<sf::Vector2f>(BetterInput::getCursorPosition(bw.window)) + bw.view.getCenter() - bw.view.getSize().componentWiseMul({ 0.5f,0.5f });
            cursorSprite.setPosition(mousePos);

            if (BetterInput::keyPressed(BetterInput::Key::MouseLeft, 1)) {
                if (BetterInput::keyPressed(BetterInput::Key::MouseLeft, 0)) {
                    lastPenPoint = mousePos;
                    //tempDrawRendTex.draw(cursorSprite);
                }
                sf::Vector2f distVec = mousePos - lastPenPoint;
                float dist = sqrt(distVec.x * distVec.x + distVec.y * distVec.y);
                if (dist==0) dist = 1.f;
              //  if (dist != 0) {
                    unsigned int distInt = static_cast<unsigned int>(ceil(dist));
                    sf::Vector2f distVecNormal = distVec.componentWiseDiv({ dist,dist });
                    sf::Vector2f incPos = lastPenPoint;
                    for (unsigned int n = 0; n <= (distInt - 1); n++) {
                        cursorSprite.setPosition(incPos);
                        tempDrawRendTex.draw(cursorSprite);
                        incPos += distVecNormal;
                    }

                   // incPos -= distVecNormal;
                    lastPenPoint = mousePos;

                    tempDrawRendTex.display();
                    tempDrawSprite.setTexture(tempDrawRendTex.getTexture());
                
            }
            else {
                canvasRendTex.draw(cursorSprite);
            }

            if (BetterInput::keyPressed(BetterInput::Key::B, 0)) {
                tempDrawRendTex.clear({0,0,0,0});
            }

            canvasRendTex.draw(tempDrawSprite);
            //canvasRendTex.draw(cursorSprite);//rename rendTex to canvasRenderTexture
            canvasRendTex.display();
            canvasRendSprite.setTexture(canvasRendTex.getTexture());
        }
        else {
            bw.window.setMouseCursorVisible(1);
        }


        worldRendTex.draw(canvasRendSprite);

        worldRendTex.setView(worldView);
        worldRendTex.display();

        worldRendSprite.setTexture(worldRendTex.getTexture());

        bw.window.draw(worldRendSprite);


        if (notificationTextTimer)
        {
            notificationTextTimer--;
            if (notificationTextTimer < notificationTextTimerStart) {
                notificationText.setFillColor({255,255,255,static_cast<std::uint8_t>(255.f*(static_cast<float>(notificationTextTimer)/static_cast<float>(notificationTextTimerStart)))});
                notificationText.setOutlineColor({0,0,0,static_cast<std::uint8_t>(255.f*(static_cast<float>(notificationTextTimer)/static_cast<float>(notificationTextTimerStart)))});
            }
            bw.window.draw(notificationText);
        }


       // if (BetterInput::keyPressed(BetterInput::Key::MouseMiddle, 0))
       //     panCam = !panCam;


      //  if (panCam)
     //       bw.view.move((mousePos - bw.view.getCenter()).componentWiseMul({ 0.04f * 1.08f,0.04f * 1.92f }));


        bw.window.setView(bw.view);
        bw.window.display();

    }
}
