#include <iostream>
#include <vector>
#include <algorithm>
#include <optional>
#include <iostream>
#include <map>
#include <cassert>
#include <random>
#include <string>
#include <set>

using Value = std::string;
using SplitedValues = std::vector< std::pair<Value, size_t> >;
const size_t InvalidIndex = std::numeric_limits<size_t>::max();

template <typename It>
Value mergeValue(It begin, It end) {
    Value result;
    for (  ; begin != end; ++begin)
        result += *begin;

    return result;
}

std::vector< Value > splitValue(const Value& v) {
    std::vector< Value > result;
    for ( auto c : v )
        result.push_back( Value(1, c) );

    return result;
}



SplitedValues splitValues(const std::vector< Value >& vals, size_t index = 0) {
    SplitedValues result;
    for ( auto& v : vals ) {
        for ( auto& v1 : splitValue(v) )
            result.emplace_back( v1, index );
        ++index;
    }
    return result;
}

std::vector< Value > mergeValues( const std::vector< std::pair<Value, size_t> >& splitedValues, std::vector<size_t>& indexes ) {
    std::vector< Value > result;

    auto it = splitedValues.begin();
    for ( ; it != splitedValues.end(); ) {
        auto end = it + 1;
        if ( it->second != InvalidIndex )
        {
            end = std::find_if( end, splitedValues.end(), [it](const std::pair<Value, size_t>& x) {
                return x.second != it->second;
            });
        }

        indexes.push_back( it - splitedValues.begin() );

        std::vector<Value> temp;
        for ( ; it != end; ++it )
            temp.push_back( it->first );

        result.push_back( mergeValue(temp.begin(), temp.end()) );
    }

    return result;
}

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

struct Container {

    struct ExtendedIndex {
        size_t index;
        size_t group;
    };

    template <typename T>
    size_t forEachSlot(size_t index, T f) const {
        size_t result = 0;
        for ( auto& pair : slotsToIndexes ) {
            if ( pair.second.index == index ) {
                f( pair.first );
                ++result;
            }
        }

        return result;
    }

    size_t findSlot(const size_t index) const {
        auto foundSlotToIndex = std::find_if( slotsToIndexes.begin(), slotsToIndexes.end(), [index](const auto& pair) {
            return pair.second.index == index;
        } );

        if ( foundSlotToIndex == slotsToIndexes.end() )
            return InvalidIndex;


        return foundSlotToIndex->first;
    }

    void onValuesChanged() {
        std::cout << "onValuesChanged\n";

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
                removedIndexes.push_back( found->second.index );
            }

            if ( slotToNewIndex.second.index != InvalidIndex && slotToNewIndex.second.index < index ) {
                assert(newIndex);

                --newIndex;
            }
        }

        std::sort( removedIndexes.begin(), removedIndexes.end() );
        for ( auto it = removedIndexes.begin(); it != removedIndexes.end() && *it < newIndex; ++it, ++newIndex );

        return newIndex;
    }

    void reassignGroups(SplitedValues& splitedValues) {
        size_t maxGroup = 0;
        for ( auto& slotToIndex : slotsToIndexes ) {
            if ( slotToIndex.second.group != InvalidIndex ) {
                maxGroup = std::max( maxGroup, slotToIndex.second.group );
            }
        }

        for ( auto& v : splitedValues ) {
            v.second += maxGroup + 1;
        }

        for ( auto& slotToIndex : slotsToIndexes ) {
            if ( slotToIndex.second.group != InvalidIndex && slotToIndex.second.index < splitedValues.size() ) {
                splitedValues[ slotToIndex.second.index ].second = slotToIndex.second.group;
            }
        }
    }

    void applyCommand(const Command& command) {

        if ( command.value && command.value == "2" ) {
            int x = 0;
            ++x;
        }

        auto foundSlotToIndex = slotsToIndexes.find( command.slotId );
        auto foundSlotToNewIndex = slotsToNewIndexes.find( command.slotId );
        

        
        auto splitedValues = splitValues( values );
        reassignGroups( splitedValues );

        auto eraseImpl = [&]() {
            const auto index = foundSlotToIndex->second.index;
            splitedValues.erase( splitedValues.begin() + index );

            slotsToIndexes.erase( foundSlotToIndex );
            foundSlotToIndex = slotsToIndexes.end();

            for ( auto& slotToIndex : slotsToIndexes ) {
                if ( slotToIndex.second.index > index )
                    --slotToIndex.second.index;
            }
        };

        auto insertImpl = [&]() {
            auto index = calculateNewIndex( foundSlotToNewIndex->second.index );
            assert(command.value);
            assert(index <= splitedValues.size());

            index = std::min(index, splitedValues.size());
            
            splitedValues.insert( splitedValues.begin() + index, { *command.value, foundSlotToNewIndex->second.group } );

            for ( auto& slotToIndex : slotsToIndexes ) {
                if ( slotToIndex.second.index >= index )
                    ++slotToIndex.second.index;
            }

            slotsToIndexes.emplace( foundSlotToNewIndex->first, ExtendedIndex{ index, foundSlotToNewIndex->second.group } );
            slotsToNewIndexes.erase( foundSlotToNewIndex );
            foundSlotToNewIndex = slotsToNewIndexes.end();
        };

        if ( foundSlotToIndex != slotsToIndexes.end() ) {
            if ( !command.value ) {
                eraseImpl();
            }
            else {
                if ( foundSlotToNewIndex == slotsToNewIndexes.end() || foundSlotToNewIndex->second.index == InvalidIndex ) {
                    splitedValues[ foundSlotToIndex->second.index ].first = *command.value;
                   // slotsToNewIndexes.erase( foundSlotToNewIndex );
                }
                else {
                    eraseImpl();
                    insertImpl();
                }
            }

            slotsToNewIndexes.erase( command.slotId );

            std::vector<size_t> indexes;
            values = mergeValues(splitedValues, indexes);

            onValuesChanged();
        }
        else if ( command.value ) {

            if ( foundSlotToNewIndex == slotsToNewIndexes.end() ) {
                slotsToIndexes.emplace( getFreeSlot(), ExtendedIndex{ values.size(), {} } );
                splitedValues.emplace_back( *command.value, InvalidIndex );
            }
            else {
                insertImpl();
            }

            std::vector<size_t> indexes;
            values = mergeValues(splitedValues, indexes);

            onValuesChanged();
        }
        else {
            std::cout << "trying to remove unexisting value" << std::endl;
        }
    }

    std::vector<Command> set(const std::vector<Value>& newValues) {
        slotsToNewIndexes.clear();

        auto newSplitedValues = splitValues( newValues );
        auto splitedValues = splitValues( values );

        std::vector<Command> result;

        std::vector<size_t> addedValueIndexes;

        std::vector<std::pair<size_t, size_t> > existingValueIndexes;
        existingValueIndexes.reserve( newSplitedValues.size() );

        std::vector<size_t> indexes;
        indexes.reserve(splitedValues.size());
        while ( indexes.size() < splitedValues.size() )
            indexes.push_back( indexes.size() );

        for ( size_t i  = 0; i < newSplitedValues.size(); ++i ) {
            auto found = std::find_if(indexes.begin(), indexes.end(), [&](size_t index)
            {
                return splitedValues[index].first == newSplitedValues[i].first;
            });
            if ( found == indexes.end() ) {
                addedValueIndexes.push_back( i );
            }
            else {
                existingValueIndexes.emplace_back(*found, i);
                indexes.erase(found);
            }
        }

        std::map< size_t, ExtendedIndex > newSlotToIndexes;

        std::vector< std::pair<Value, size_t> > fixedValues;
        fixedValues.reserve( splitedValues.size() );

        size_t j = 0;


        std::vector<size_t> changedIndexes;

        for ( size_t i = 0; i < splitedValues.size(); ++i ) {
            if ( j < indexes.size() && i == indexes[j] ) {
                if ( addedValueIndexes.empty() ) {
                    auto slot = findSlot(i);
                    assert(slot != InvalidIndex);

                    {
                        result.push_back( Command{ slot, {} } );
                        slotsToNewIndexes.emplace( slot, ExtendedIndex{ InvalidIndex, InvalidIndex } );
                        newSlotToIndexes.emplace( slot, ExtendedIndex{ fixedValues.size(), InvalidIndex } );
                    }
                }
                else {
                    auto slot = findSlot(i);
                    assert(slot != InvalidIndex);

                    {
                        result.push_back( Command{ slot, newSplitedValues[addedValueIndexes.back()].first } );

                        auto group = newSplitedValues[addedValueIndexes.back()].second;

                        if (group == 10) {
                            int x = 2;
                        }

                        slotsToNewIndexes.emplace( slot, ExtendedIndex{ addedValueIndexes.back(), group } );
                        newSlotToIndexes.emplace( slot, ExtendedIndex{ fixedValues.size(), InvalidIndex } );
                    }

                    addedValueIndexes.pop_back();
                }

                //assert(slotsCount);

                fixedValues.push_back( splitedValues[ i ] );

                changedIndexes.push_back( i );
                ++j;
            }
            else {
                auto slot = findSlot(existingValueIndexes[ i - j ].first);
                assert(slot != InvalidIndex);


                {
                    if (existingValueIndexes[ i - j ].second == 11) {
                        int x = 2;
                    }

                    newSlotToIndexes.emplace( slot, ExtendedIndex{ fixedValues.size(), newSplitedValues[ existingValueIndexes[ i - j ].second ].second } );
                }

                fixedValues.push_back( splitedValues[ existingValueIndexes[ i - j ].first ] );
                changedIndexes.push_back( existingValueIndexes[ i - j ].first );
            }
        }

        slotsToIndexes = std::move( newSlotToIndexes );

        std::vector<size_t> tempIndexes;
        reassignGroups( fixedValues );
        values = mergeValues( fixedValues, tempIndexes );

        //for ( auto& v  )

        onValuesChanged();

        for ( auto addedIndex : addedValueIndexes ) {
            const auto slot = getFreeSlot();

            if (newSplitedValues[addedIndex].second == 11) {
                int x = 2;
            }

            slotsToNewIndexes.emplace( slot, ExtendedIndex{ addedIndex, newSplitedValues[addedIndex].second } );

            result.push_back( Command{ slot, newSplitedValues[addedIndex].first } );
        }

        return result;
    }

    std::map< size_t, ExtendedIndex > slotsToIndexes;
    std::map< size_t, ExtendedIndex > slotsToNewIndexes;

    std::vector<Value> values;
};



int main()
{
    Container c;
    std::random_device rd;
    std::mt19937 g(0);

    auto doTest = [&](const std::vector<Value>& values) {
        auto commands = c.set(values);
        std::shuffle( commands.begin(), commands.end(), g );

        for ( auto& cmd : commands ) {
            c.applyCommand(cmd);
          //  assert( c.values.size() == c.slotsToIndexes.size() );
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

        if (i == 74) {
            int x = 1;
            ++x;
        }
        std::vector<Value> values( rand() % 17 );
        for ( auto& v : values ) {
            v = std::to_string(rand() % 24);
            if ( v == "10" )
                v = "xy";
            else if ( v == "11" )
                v = "x";
            else if ( v == "12" )
                v = "y";
            else if ( v == "13" )
                v = "xuy xuy";
        }

        doTest( values );
    }

    std::cout << "Test finished" << std::endl;

    return 0;
}
