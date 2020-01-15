#include <iostream>
#include <vector>
#include <algorithm>
#include <optional>
#include <iostream>
#include <map>
#include <cassert>
#include <random>
#include <string>

using Value = std::string;

enum class CommandType {
    Add,
    Move,
    Remove
};

struct Command {
    size_t slotId;
    std::optional<Value> value;
   // CommandType type;
};

size_t getValueCost(const Value& value) {
    return  value == "x" ? 2 : 1;
}

const size_t InvalidIndex = size_t(-1);

struct Container {

    template <typename T>
    size_t forEachSlot(size_t index, T f) const {
        size_t result = 0;
        for ( auto& pair : slotsToIndexes ) {
            if ( pair.second == index ) {
                f( pair.first );
                ++result;
            }
        }

        return result;
    }

    std::optional<size_t> findSlot(const size_t index) const {
        auto foundSlotToIndex = std::find_if( slotsToIndexes.begin(), slotsToIndexes.end(), [index](const auto& pair) {
            return pair.second == index;
        } );

        return foundSlotToIndex == slotsToIndexes.end() ? std::optional<size_t>() : foundSlotToIndex->first;
    }

    void onValuesChanged() {
        std::cout << "onValuesChanged\n";
        assert( values.size() == slotsToIndexes.size() );
        for ( size_t i = 0; i < values.size(); ++i) {
            std::cout << "  slot: ";

            forEachSlot(i, [&](size_t slot)
            {
                std::cout << slot << " ";
            });

            std::cout << " value: " << values[ i ] << "\n";
        }

        std::cout << std::endl;
    }

    size_t getFreeSlot() const {
        for ( size_t i = 1; ; ++i ) {
            if ( slotsToIndexes.find( i ) == slotsToIndexes.end() && slotsToNewIndexes.find(i) == slotsToNewIndexes.end() )
                return i;
        }
    }

    size_t calculateNewIndex( size_t index ) const {
        size_t newIndex = index;

        std::vector< size_t > removedIndexes;
        for (auto& slotToNewIndex : slotsToNewIndexes)  {
            const auto found = slotsToIndexes.find( slotToNewIndex.first );
            if ( found != slotsToIndexes.end() ) {
                removedIndexes.push_back( found->second );
            }

            if ( slotToNewIndex.second != InvalidIndex && slotToNewIndex.second < index ) {
                assert(newIndex);
                --newIndex;
            }
        }

        std::sort( removedIndexes.begin(), removedIndexes.end() );
        for ( auto removedIndex : removedIndexes ) {
            if ( removedIndex < newIndex )
                ++newIndex;
            else
                break;
        }

        return newIndex;
    }

    void applyCommand(const Command& command) {

        auto foundSlotToIndex = slotsToIndexes.find( command.slotId );
        auto foundSlotToNewIndex = slotsToNewIndexes.find( command.slotId );

        auto eraseImpl = [&]() {
            const auto index = foundSlotToIndex->second;
            values.erase( values.begin() + index );

            slotsToIndexes.erase( foundSlotToIndex );
            foundSlotToIndex = slotsToIndexes.end();

            for ( auto& slotToIndex : slotsToIndexes ) {
                if ( slotToIndex.second > index && slotToIndex.second != InvalidIndex )
                    --slotToIndex.second;
            }
        };

        auto insertImpl = [&]() {
            auto index =  calculateNewIndex( foundSlotToNewIndex->second );
            assert(command.value);
            assert(index <= values.size());

            index = std::min(index, values.size());

            values.insert( values.begin() + index, *command.value );

            for ( auto& slotToIndex : slotsToIndexes ) {
                if ( slotToIndex.second >= index && slotToIndex.second != InvalidIndex )
                    ++slotToIndex.second;
            }

            slotsToIndexes.emplace( foundSlotToNewIndex->first, index );
            slotsToNewIndexes.erase( foundSlotToNewIndex );
            foundSlotToNewIndex = slotsToNewIndexes.end();
        };

        if ( foundSlotToIndex != slotsToIndexes.end() ) {
            if ( !command.value ) {
                eraseImpl();
            }
            else {
                if ( foundSlotToNewIndex == slotsToNewIndexes.end() || foundSlotToNewIndex->second == InvalidIndex ) {
                    values[ foundSlotToIndex->second ] = *command.value;
                   // slotsToNewIndexes.erase( foundSlotToNewIndex );
                }
                else {
                    eraseImpl();
                    insertImpl();
                }
            }

            slotsToNewIndexes.erase( command.slotId );

            onValuesChanged();
        }
        else if ( command.value ) {

            if ( foundSlotToNewIndex == slotsToNewIndexes.end() ) {
                slotsToIndexes.emplace( getFreeSlot(), values.size() );
                values.push_back( *command.value );
            }
            else {
                insertImpl();
            }

            onValuesChanged();
        }
        else {
            std::cout << "trying to remove unexisting value" << std::endl;
        }
    }

    std::vector<Command> set(const std::vector<Value>& newValues) {
        std::vector<Command> result;

        std::vector<size_t> addedValueIndexes;

        std::vector<size_t> existingValueIndexes;
        existingValueIndexes.reserve( newValues.size() );

        std::vector<size_t> indexes;
        indexes.reserve(values.size());
        while ( indexes.size() < values.size() )
            indexes.push_back( indexes.size() );

        for ( size_t i  = 0; i < newValues.size(); ++i ) {
            auto found = std::find_if(indexes.begin(), indexes.end(), [&](size_t index)
            {
                return values[index] == newValues[i];
            });
            if ( found == indexes.end() ) {
                addedValueIndexes.push_back( i );
            }
            else {
                existingValueIndexes.push_back(*found);
                indexes.erase(found);
            }
        }

        std::map< size_t, size_t > newSlotToIndexes;
        std::vector< Value > fixedValues;
        fixedValues.reserve( values.size() );

        size_t j = 0;
        for ( size_t i = 0; i < values.size(); ++i ) {
            if ( j < indexes.size() && i == indexes[j] ) {
                const auto slotsCount = forEachSlot( i, [&](size_t slot)
                {
                    newSlotToIndexes.emplace( slot, fixedValues.size() );

                    if ( addedValueIndexes.empty() ) {
                        result.push_back( Command{ slot, {} } );
                        slotsToNewIndexes.emplace( slot, InvalidIndex );
                    }
                    else {
                        result.push_back( Command{ slot, newValues[addedValueIndexes.back()] } );
                        slotsToNewIndexes.emplace( slot, addedValueIndexes.back() );

                        addedValueIndexes.pop_back();
                    }
                });

                assert(slotsCount);

                fixedValues.push_back( values[ i ] );
                ++j;
            }
            else {
                const auto slotsCount = forEachSlot( existingValueIndexes[ i - j ], [&](size_t slot)
                {
                    newSlotToIndexes.emplace( slot, fixedValues.size() );
                });

                assert(slotsCount);

                fixedValues.push_back( values[ existingValueIndexes[ i - j ] ] );
            }
        }

        values = std::move( fixedValues );
        slotsToIndexes = std::move( newSlotToIndexes );

        onValuesChanged();

        for ( auto addedIndex : addedValueIndexes ) {
            const auto slot = getFreeSlot();
            slotsToNewIndexes.emplace( slot, addedIndex );

            result.push_back( Command{ slot, newValues[addedIndex] } );
        }

        return result;
    }

    std::map< size_t, size_t > slotsToIndexes;
    std::map< size_t, size_t > slotsToNewIndexes;
    std::vector<Value> values;
};



int main()
{
    Container c;
    std::random_device rd;
    std::mt19937 g(rd());

    auto doTest = [&](const std::vector<Value>& values) {
        auto commands = c.set(values);
        std::shuffle( commands.begin(), commands.end(), g );

        for ( auto& cmd : commands ) {
            c.applyCommand(cmd);
            assert( c.values.size() == c.slotsToIndexes.size() );
        }



        assert( c.slotsToNewIndexes.empty() );
        assert( c.values == values );
    };

    doTest({ "a", "b" });
    doTest({ "c", "b", "c", "d"});
    doTest({ "d", "c", "c", "b"});
    doTest({ "a", "d", "c", "b", "b", "c"});
    doTest({ "k", "a", "b", "c", "b", "d", "c"});
    doTest({ "c", "a", "u"} );
    doTest({ } );

    for ( int i = 0; i < 30000; ++i ) {
        std::vector<Value> values( rand() % 17 );
        for ( auto& v : values ) {
            v = std::to_string(rand() % 10);
        }

        doTest( values );
    }

    std::cout << "Test finished" << std::endl;

    return 0;
}
