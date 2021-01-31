#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <sstream>
#include <regex>
#include <Windows.h>
#include <shlobj.h>
#include <direct.h>

#include "rapidjson/filereadstream.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"


using namespace rapidjson;
using namespace std;

/*  Chaotic Deck Randomizer

    Number of cards currently added in Recode (# added/# total) 
    ***THIS MAY NOT BE 100% ACCURATE, missing.txt from Chaotic discord is not fully updated***
    # total added: 1083/1225
        # creatures: 427/475
            UW: 100/110
            OW: 111/120
            D: 81/92
            Mip: 92/96
            M'ar: 39/53
            Gen: 4/4
        # battlegear: 126/155
        # attacks: 252/260
        # mugic: 161/189
            UW: 26/32
            OW: 31/34
            D: 29/32
            Mip: 28/33
            M'ar: 10/13
            Gen: 37/45
        # locations: 115/146


    General idea of randomization 
        1) pick random creatures
            1.1) if creature is loyal, pick rest of creatures from same tribe 
            1.2) if creature is loyal and m'arrillian, pick rest of creatures from same tribe or minions
            1.3) if creature is legendary, do not allow for more legendary creatures
            1.4) if card is unique, do not pick a duplicate 
                 (may need hard coding for multiple versions of same card where one is unique and another is not, ex: Headmaster Ankhyja, Headmaster Ankhyja SOTA)

       2) pick random attacks
            2.1) if card is unique, do not pick a duplicate 
            2.2) keep track of Build Points (20), keep build point count after adding each attack
                 if BP is 20 and less than 20 attacks are added, only add attacks with 0 BP
                 same if BP is 18, only add atacks with 2 BP or less; etc

       3) pick random locations 
            3.1) if card is unique, do not pick a duplicate 

       4) pick random battlegear (must be chosen after selecting creatures)
            4.1) if card is unique, do not pick a duplicate 
            4.2) if card if legendary, do not add unless chosen creatures match tribe 

       5) pick random mugic
            5.1) if card is unique, do not pick a duplicate
            method one (must be chosen after selecting creatures): 
                5.2a) only add mugic for which there exists a matching tribe from the chosen creatures (and generic) 
            method two: **CURRENTLY IN USE**
                5.2b) add any mugic with disregard for already chosen creatures tribe (would lead to higher chance to not be able cast mugic) 


    Balancing
        1) There may be balance problems when Loyal creatures are selected early in the process.
           This not only adds a (usually) very powerful creature to your random deck (ex: Chaor, Takinom, Ankhyja),
           but also ensures that the rest of the creatures to be randomly added are all the same tribe. 
           This can make a "random" deck not very random at all compared to the more common mixed tribe results. 
           -possible solutions: 1) ban loyal cards 
                                2) ???
        2) Mugic are currently totally random which may cause the inability to play all or any mugic. 
           -possible solutions: 1) only add mugic that match the already picked tribes + generics 
        3) Attacks may be TOO random. Most decks result in mixed armies that don't share elements/etc. Even
           single tribe armies can have no common characteristics. This can make many of the random attacks
           useless or deal very little damage. 
           -possible solutions: 1) track creature elements and/or disciplines and give a weight towards attacks that match them (this would be annoying to implement)
                                2) try to create a more balanced attack deck by setting a limit to high bp attacks or giving higher weights to lower build point attacks
                                3) do nothing and embrace chaos
*/


Document open_doc() { //open chaotic.json
    // Open File
    ifstream in("chaotic.json", ios::binary);
    if (!in)
        throw std::runtime_error("Failed to open file.");

    // dont skip on whitespace
    noskipws(in);

    // Read in content
    istreambuf_iterator<char> head(in);
    istreambuf_iterator<char> tail;
    string data(head, tail);

    // Parse content
    Document d;
    d.Parse(data.c_str());

    return d;
}

int get_random(int low, int high) { //gen a random number with different seed every run 
    random_device rd;   
    mt19937 mt(rd());
    uniform_real_distribution<double> dist(low, high); //range is from low to high 
    int random = dist(mt);
    return random;
}

string parse_name(string card_name) { //remove extra titles from cards to help check for duplicates in the deck (ex: Headmaster Ankhyja, Seeker of the Arts -> Headmaster Ankhyja)
    vector<string> result;
    stringstream ss(card_name);
    
    while (ss.good()) {
        string substr;
        getline(ss, substr, ',');
        substr = regex_replace(substr, regex("^ +"), ""); //remove leading whitespace from strings since splitting card names with ", " leaves a " " space in the sub-title
        result.push_back(substr);
    }

    return result[0];
}

vector<string> get_locations(const Document& d) {
    int loc_count = 0;
    vector<string> locations;   //vector of added locations 

    while (loc_count < 10) {    // until 10 locations are picked 
        int random = get_random(0, 145); // gen a random number from 0 to 145 (146 total location cards)
        //cout << "Random: " << random << endl;

        //iterate through cards until you reach random card
        int count = 0;  
        for (auto itr = d["locations"].MemberBegin(); itr != d["locations"].MemberEnd(); ++itr) {
            bool dont_add = false; //if there is some problem with adding a card (2 copies/unique), trigger this to stop the loop and gen a new random number

            if (count == random) { //stop when the random card is reached
                //get card attribs 
                string card, added, unique, card_parsed;
                card = itr->name.GetString(); //full card name 
                card_parsed = parse_name(card); //parsed name 
                Value::ConstMemberIterator card_itr = d["locations"][itr->name.GetString()].FindMember("added");
                added = card_itr->value.GetString(); //is card in recode 
                card_itr = d["locations"][itr->name.GetString()].FindMember("unique");
                unique = card_itr->value.GetString(); //is card unique 

                if (added == "1") {   //if current card is in recode, try to add card to locations 
                    int duplicates = 0; //count for tracking copies of current card already in locations 

                    //check for more than one copy of card and if at least one is Unique
                    for (string loc : locations) { //check if base card name is already in locations (ex: Glacier Plains and Glacier Plans, M'arrillian Melting Camp are both considered "Glacier Plains")
                        string loc_parsed = parse_name(loc);

                        if (loc_parsed == card_parsed) {   //if card with same base name in locations, check if that card is unique 
                            Value::ConstMemberIterator check_unique = d["locations"][loc.c_str()].FindMember("unique");
                            string loc_unique = check_unique->value.GetString();

                            if (loc_unique == "1" || unique == "1") { //if card in locations is unique or current card is unique, dont add current card 
                                cout << "\tLOCATION: more than one copy of card and at least one is Unique" << endl;
                                dont_add = true;
                                break;
                            }      
                            duplicates++; //increase # of copies of current card in locations
                        }
                    }

                    if (duplicates <= 1 && dont_add == false) {   //if there is at most 1 duplicate already in locations, add card
                        cout << "\tLOCATION: added" << endl;
                        locations.push_back(card);
                        loc_count++;
                    }

                    else if (duplicates >= 2) {  //else there are already 2 copies of the card, so dont add
                        cout << "\tLOCATION: 2 copies aleady" << endl;
                        dont_add = true;
                        break;
                    }
                }

                else {  //current card is not added in recode
                    cout << "\tLOCATION: not in recode" << endl;
                    dont_add = true;
                    break;
                }
            }

            if (dont_add)   //if dont_add is triggered to be true, break out of loop for current card and find another random card to add
                break;
            else 
                count++;    //increase # of cards looked through before reaching the random card
        }
    }
    //print locations 
    cout << "\n\tLOCATIONS" << endl;
    int c = 1;
    for (auto l : locations) 
        cout << "\t" << c++ << ": " << l << endl;
    return locations;
}

vector<string> get_attacks(const Document& d) { //there is a small chance that the 20 attacks are below the 20 bp limit, not sure if to leave it or make it so it always reaches 20
    int total_bp = 0;
    int attack_count = 0;
    vector<string> attacks;

    while (attack_count < 20) {    // until 20 attacks are picked 
        int random = get_random(0, 259); // gen a random number from 0 to 259 (260 total attacks)
        //cout << "Random: " << random << endl;

        //iterate through cards until you reach random card
        int count = 0;
        for (auto itr = d["attacks"].MemberBegin(); itr != d["attacks"].MemberEnd(); ++itr) {
            bool dont_add = false; //if there is some problem with adding a card (2 copies/unique), trigger this to stop the loop and gen a new random number

            if (count == random) { //stop when the random card is reached
                //get card attribs 
                string card, added, unique;
                int bp; 
                card = itr->name.GetString(); //full card name 
                Value::ConstMemberIterator card_itr = d["attacks"][itr->name.GetString()].FindMember("added");
                added = card_itr->value.GetString(); //is card in recode 
                card_itr = d["attacks"][itr->name.GetString()].FindMember("unique");
                unique = card_itr->value.GetString(); //is card unique 
                card_itr = d["attacks"][itr->name.GetString()].FindMember("bp");
                bp = stoi(card_itr->value.GetString()); //get attack's build point cost
                cout << card << ", " << bp << endl;

                if (added == "1") {   //if current card is in recode and adding it won't go over 20 build points, try to add card to attacks 
                    if (total_bp + bp > 20) { //if adding card goes over 20 bp, dont add
                        cout << "\tATTACK: over 20 bp" << endl;
                        dont_add = true;
                        break;
                    }

                    int duplicates = 0; //count for tracking copies of current card already in locations 

                    //check for more than one copy of card and if at least one is Unique
                    for (string atk : attacks) {

                        if (atk == card) {   //if card with same name in attacks, check if that card is unique 
                            if (unique == "1") { //if current card is unique, dont add current card 
                                cout << "\tATTACK: more than one copy of card and at least one is Unique" << endl;
                                dont_add = true;
                                break;
                            }
                            duplicates++; //increase # of copies of current card in attacks
                        }
                    }

                    if (duplicates <= 1 && dont_add == false) {   //if there is at most 1 duplicate already in attack, add card and inc total_bp
                        cout << "\tATTACK: added" << endl;
                        attacks.push_back(card);
                        attack_count++;
                        total_bp += bp;
                        cout << total_bp << endl;
                    }

                    else if (duplicates >= 2) {  //else there are already 2 copies of the card, so dont add
                        cout << "\tATTACK: 2 copies aleady" << endl;
                        dont_add = true;
                        break;
                    }
                }

                else {  //current card is not added in recode
                    cout << "\tATTACK: not in recode" << endl;
                    dont_add = true;
                    break;
                }
            }

            if (dont_add)   //if dont_add is triggered to be true, break out of loop for current card and find another random card to add
                break;
            else
                count++;    //increase # of cards looked through before reaching the random card
        }
    }

    //print attacks 
    cout << "\n\tATTACKS" << endl;
    int c = 1;
    for (auto a : attacks)
        cout << "\t" << c++ << ": " << a << endl;
    cout << "\t Total BP: " << total_bp << endl;
    return attacks;
}

vector<string> get_creatures(const Document& d) {
    vector<string> creatures;   //track added creatures
    int creature_count = 0;
    string loyalty = ""; //if the first creature picked is loyal to a tribe, set this to that tribe and only choose creatures from that tribe (or minions if m'arrillian)

    while (creature_count < 6) {    // until 6 creatures are picked 
        int random = get_random(0, 474); // gen a random number from 0 to 474 (475 total creatures)
        //cout << "Random: " << random << endl;

        //iterate through cards until you reach random card
        int count = 0;
        for (auto itr = d["creatures"].MemberBegin(); itr != d["creatures"].MemberEnd(); ++itr) {
            bool dont_add = false; //if there is some problem with adding a card (2 copies/unique), trigger this to stop the loop and gen a new random number

            if (count == random) { //stop when the random card is reached

                //get card attribs 
                string card, card_parsed, added, unique, minion, loyal, tribe;
                card = itr->name.GetString(); //full card name 
                card_parsed = parse_name(card); //parsed name 
                Value::ConstMemberIterator card_itr = d["creatures"][itr->name.GetString()].FindMember("added");
                added = card_itr->value.GetString(); //is card in recode 
                card_itr = d["creatures"][itr->name.GetString()].FindMember("unique");
                unique = card_itr->value.GetString(); //is card unique 
                card_itr = d["creatures"][itr->name.GetString()].FindMember("loyal");
                loyal = card_itr->value.GetString(); //get card's tribe loyalty, if any
                card_itr = d["creatures"][itr->name.GetString()].FindMember("minion");
                minion = card_itr->value.GetString(); //if card is a minion
                card_itr = d["creatures"][itr->name.GetString()].FindMember("tribe");
                tribe = card_itr->value.GetString(); //card's tribe
                cout << card << endl;

                if (added == "1") {   //if current card is in recode and adding it won't go over 20 build points, try to add card to attacks
                    if (loyal != "") { //if current card is loyal
                        cout << "LOYAL" << endl;
                        if (creature_count == 0) { //if this is the first card, set loyalty for rest of creatures 
                            cout << "\tCREATURE: first creature loyal to " << loyal << endl;
                            loyalty = loyal;
                        }

                        for (auto cr : creatures) { //check if any other creatures added before would disallow the current card b/c of loyalty
                            card_itr = d["creatures"][cr.c_str()].FindMember("tribe");  //get added creature's tribe, loyalty, and check if its a minion
                            string added_creature_tribe = card_itr->value.GetString();
                            card_itr = d["creatures"][cr.c_str()].FindMember("minion");
                            string added_creature_minion = card_itr->value.GetString();
                            card_itr = d["creatures"][cr.c_str()].FindMember("loyal");
                            string added_creature_loyal = card_itr->value.GetString();

                            if (added_creature_loyal != "") { //if prev added creature is loyal
                                if (added_creature_loyal == "m'arrillian" && (tribe != "m'arrillian" && minion != "1")) { //if prev added creature is loyal to m'ar and current creature is not m'ar or minion, dont add
                                    cout << "\tCREATURE: 1prev creature loyal to m'arrillian and curr creature not m'arrillian or minion" << endl;
                                    dont_add = true;
                                    break;
                                }
                                else if (added_creature_loyal != "m'arrillian" && added_creature_loyal != tribe) { //else if prev added creature was another tribe that doesn't match tribe of current card, don't add 
                                    cout << "\tCREATURE: 1prev creature is " << added_creature_tribe << " and cur creature is loyal to " << loyal << endl;
                                    dont_add = true;
                                    break;
                                }
                            }
                            else {  //else prev creature is not loyal 
                                //if a tribe from added creatures doesn't match current creature, dont add 
                                if (added_creature_tribe == "m'arrillian" && (tribe != "m'arrillian" && minion != "1")) {  //if prev added creature is m'ar and current creature is not m'ar or minion, dont add
                                    cout << "\tCREATURE: 2prev creature m'arrillian and curr loyal creature not m'arrillian or minion" << endl;
                                    dont_add = true;
                                    break;
                                }
                                else if (added_creature_tribe != tribe) { //else if prev added creature was another tribe that doesn't match loyalty of current card, don't add 
                                    cout << "\tCREATURE: 2prev creature is " << added_creature_tribe << " and cur creature is loyal to " << loyal << endl;
                                    dont_add = true;
                                    break;
                                }
                            }
                        }
                    }
                    else { //if current card not loyal, check if any prev added cards are loyal and if cur card matches tribe
                        cout << "NOT LOYAL" << endl;
                        for (auto cr : creatures) { //check if any other creatures added before would disallow the current card b/c of loyalty
                            card_itr = d["creatures"][cr.c_str()].FindMember("tribe");  //get added creature's tribe, loyalty, and check if its a minion
                            string added_creature_tribe = card_itr->value.GetString();
                            card_itr = d["creatures"][cr.c_str()].FindMember("minion");
                            string added_creature_minion = card_itr->value.GetString();
                            card_itr = d["creatures"][cr.c_str()].FindMember("loyal");
                            string added_creature_loyal = card_itr->value.GetString();

                            if (added_creature_loyal != "") { //if prev added creature is loyal
                                if (added_creature_loyal == "m'arrillian" && (tribe != "m'arrillian" && minion != "1")) { //if prev added creature is loyal to m'ar and current creature is not m'ar or minion, dont add
                                    cout << "\tCREATURE: 3prev creature loyal to m'arrillian and curr creature not m'arrillian or minion" << endl;
                                    dont_add = true;
                                    break;
                                }
                                else if (added_creature_loyal != "m'arrillian" && added_creature_loyal != tribe) { //else if prev added creature was another tribe that doesn't match tribe of current card, don't add 
                                    cout << "\tCREATURE: 3prev creature is " << added_creature_tribe << " and cur creature is loyal to " << loyal << endl;
                                    dont_add = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (dont_add) //if dont_add is triggered in 2 the inner for each loops above, break out of while loop for current card early 
                        break;

                    int duplicates = 0; //count for tracking copies of current card already in creatures 

                    //check for more than one copy of card and if at least one is Unique
                    for (string cr : creatures) {
                        string cr_parsed = parse_name(cr);

                        if (cr_parsed == card_parsed) {   //if card with same base name in creatures, check if that card is unique 
                            Value::ConstMemberIterator check_unique = d["creatures"][cr.c_str()].FindMember("unique");
                            string cr_unique = check_unique->value.GetString();

                            if (cr_unique == "1" || unique == "1") { //if card in creatures is unique or current card is unique, dont add current card 
                                cout << "\tCREATURE: more than one copy of card and at least one is Unique" << endl;
                                dont_add = true;
                                break;
                            }
                            duplicates++; //increase # of copies of current card in creatures
                        }
                    }

                    if (duplicates <= 1 && dont_add == false) {   //if there is at most 1 duplicate already in creatures, add card 
                        cout << "\tCREATURE: added" << endl;
                        creatures.push_back(card);
                        creature_count++;
                    }

                    else if (duplicates >= 2) {  //else there are already 2 copies of the card, so dont add
                        cout << "\tCREATURE: 2 copies aleady" << endl;
                        dont_add = true;
                        break;
                    }
                }

                else {  //current card is not added in recode
                    cout << "\tCREATURE: not in recode" << endl;
                    dont_add = true;
                    break;
                }
            }

            if (dont_add)   //if dont_add is triggered to be true, break out of loop for current card and find another random card to add
                break;
            else
                count++;    //increase # of cards looked through before reaching the random card
        }
    }

    //print creatures 
    creatures.push_back(loyalty); //add loyalty if any to the end of the vector to use in battlegear selection
    cout << "\n\tCREATURES" << endl;
    for (int c = 1; c < 7; c++)
        cout << "\t" << c << ": " << creatures[c-1] << endl;
    return creatures;
}

vector<string> get_battlegear(const Document& d, vector<string> creatures) {
    vector<string> battlegear;
    int bg_count = 0;
    string loyalty = ""; 
    if (creatures[6] != "") { //if there is loyalty for the selected creatures, set loyalty for possible legendary battlegear 
        loyalty = creatures[6];
        cout << "\tBATTLEGEAR: loyalty - " << loyalty << endl;
    }

    while (bg_count < 6) {    // until 6 bg are picked 
        int random = get_random(0, 154); // gen a random number from 0 to 154 (155 total bg)
        //cout << "Random: " << random << endl;

        //iterate through cards until you reach random card
        int count = 0;
        for (auto itr = d["battlegear"].MemberBegin(); itr != d["battlegear"].MemberEnd(); ++itr) {
            bool dont_add = false; //if there is some problem with adding a card (2 copies/unique), trigger this to stop the loop and gen a new random number

            if (count == random) { //stop when the random card is reached

                //get card attribs 
                string card, card_parsed, added, unique;
                card = itr->name.GetString(); //full card name 
                card_parsed = parse_name(card); //parsed name 
                Value::ConstMemberIterator card_itr = d["battlegear"][itr->name.GetString()].FindMember("added");
                added = card_itr->value.GetString(); //is card in recode 
                card_itr = d["battlegear"][itr->name.GetString()].FindMember("unique");
                unique = card_itr->value.GetString(); //is card unique 
                cout << card << endl;

                if (added == "1") {   //if current card is in recode and adding it won't go over 20 build points, try to add card to attacks

                    //hard code check for legendary bg 
                    if (card == "Crown of Aa'une") {
                        if (loyalty != "m'arrillian") {    //if loyalty from selected creatures doesn't match the leg bg, don't add 
                            dont_add = true;
                            break;
                        }
                    }
                    if (card == "Dread Tread") {
                        if (loyalty != "underworld") {    
                            dont_add = true;
                            break;
                        }
                    }
                    if (card == "Hornsabre") {
                        if (loyalty != "overworld") {
                            dont_add = true;
                            break;
                        }
                    }
                    if (card == "Scepter of the Infernal Parasite ") {
                        if (loyalty != "danian") {
                            dont_add = true;
                            break;
                        }
                    }
                    if (card == "Warriors of Owayki ") {
                        if (loyalty != "mipedian") {
                            dont_add = true;
                            break;
                        }
                    }
   
                    int duplicates = 0; //count for tracking copies of current card already in battlegear 

                    //check for more than one copy of card and if at least one is Unique
                    for (string bg : battlegear) {
                        string bg_parsed = parse_name(bg);

                        if (bg_parsed == card_parsed) {   //if card with same base name in battlegear, check if that card is unique 
                            Value::ConstMemberIterator check_unique = d["battlegear"][bg.c_str()].FindMember("unique");
                            string bg_unique = check_unique->value.GetString();

                            if (bg_unique == "1" || unique == "1") { //if card in battlegear is unique or current card is unique, dont add current card 
                                cout << "\tBATTLEGEAR: more than one copy of card and at least one is Unique" << endl;
                                dont_add = true;
                                break;
                            }
                            duplicates++; //increase # of copies of current card in battlegear
                        }
                    }

                    if (duplicates <= 1 && dont_add == false) {   //if there is at most 1 duplicate already in battlegear, add card 
                        cout << "\tBATTLEGEAR: added" << endl;
                        battlegear.push_back(card);
                        bg_count++;
                    }

                    else if (duplicates >= 2) {  //else there are already 2 copies of the card, so dont add
                        cout << "\tBATTLEGEAR: 2 copies aleady" << endl;
                        dont_add = true;
                        break;
                    }
                }

                else {  //current card is not added in recode
                    cout << "\tBATTLEGEAR: not in recode" << endl;
                    dont_add = true;
                    break;
                }
            }

            if (dont_add)   //if dont_add is triggered to be true, break out of loop for current card and find another random card to add
                break;
            else
                count++;    //increase # of cards looked through before reaching the random card
        }
    }

    //print battlegear
    cout << "\n\tBATTLEGEAR" << endl;
    int c = 1;
    for (auto bg : battlegear)
        cout << "\t" << c++ << ": " << bg << endl;
    return battlegear;
}

vector<string> get_mugic(const Document& d, vector<string> creatures) { // **mugic completely random, may make it so that mugic from tribes that are not from the selected creatures cannot be chosen**
    vector<string> mugic;
    int mugic_count = 0;

    while (mugic_count < 6) {    // until 6 mugic are picked 
        int random = get_random(0, 188); // gen a random number from 0 to 188 (189 total mugic)
        //cout << "Random: " << random << endl;

        //iterate through cards until you reach random card
        int count = 0;
        for (auto itr = d["mugic"].MemberBegin(); itr != d["mugic"].MemberEnd(); ++itr) {
            bool dont_add = false; //if there is some problem with adding a card (2 copies/unique), trigger this to stop the loop and gen a new random number

            if (count == random) { //stop when the random card is reached

                //get card attribs 
                string card, card_parsed, added, unique;
                card = itr->name.GetString(); //full card name 
                card_parsed = parse_name(card); //parsed name 
                Value::ConstMemberIterator card_itr = d["mugic"][itr->name.GetString()].FindMember("added");
                added = card_itr->value.GetString(); //is card in recode 
                card_itr = d["mugic"][itr->name.GetString()].FindMember("unique");
                unique = card_itr->value.GetString(); //is card unique 
                cout << card << endl;

                if (added == "1") {   //if current card is in recode and adding it won't go over 20 build points, try to add card to attacks
                    int duplicates = 0; //count for tracking copies of current card already in battlegear 

                    //check for more than one copy of card and if at least one is Unique
                    for (string m : mugic) {
                        string m_parsed = parse_name(m);

                        if (m_parsed == card_parsed) {   //if card with same base name in battlegear, check if that card is unique 
                            Value::ConstMemberIterator check_unique = d["mugic"][m.c_str()].FindMember("unique");
                            string m_unique = check_unique->value.GetString();

                            if (m_unique == "1" || unique == "1") { //if card in battlegear is unique or current card is unique, dont add current card 
                                cout << "\tMUGIC: more than one copy of card and at least one is Unique" << endl;
                                dont_add = true;
                                break;
                            }
                            duplicates++; //increase # of copies of current card in battlegear
                        }
                    }

                    if (duplicates <= 1 && dont_add == false) {   //if there is at most 1 duplicate already in battlegear, add card 
                        cout << "\tMUGIC: added" << endl;
                        mugic.push_back(card);
                        mugic_count++;
                    }

                    else if (duplicates >= 2) {  //else there are already 2 copies of the card, so dont add
                        cout << "\tMUGIC: 2 copies aleady" << endl;
                        dont_add = true;
                        break;
                    }
                }

                else {  //current card is not added in recode
                    cout << "\tMUGIC: not in recode" << endl;
                    dont_add = true;
                    break;
                }
            }

            if (dont_add)   //if dont_add is triggered to be true, break out of loop for current card and find another random card to add
                break;
            else
                count++;    //increase # of cards looked through before reaching the random card
        }
    }

    //print mugic
    cout << "\n\tMUGIC" << endl;
    int c = 1;
    for (auto m : mugic)
        cout << "\t" << c++ << ": " << m << endl;
    return mugic;
}


int write_file(vector<string> creatures, vector<string> attacks, vector<string> battlegear, vector<string> mugic, vector<string> locations) {   // write the randomly selected cards to file so they can be imported as a deck
    ofstream deck_import_file;

    //find path to desktop of user (not sure if this works only on windows or linux as well)
    TCHAR appData[MAX_PATH];    
    if (SUCCEEDED(SHGetFolderPath(NULL,
            CSIDL_DESKTOPDIRECTORY | CSIDL_FLAG_CREATE,
            NULL,
            SHGFP_TYPE_CURRENT,
            appData))
        )
        wcout << "DESKTOP PATH: " << appData << endl; 

    deck_import_file.open(lstrcat(appData, L"\\\\chaotic_random_deck.txt"));    //concat the path to desktop with filename
    deck_import_file << "//**Import this file in chaoticrecode.com as untap.in deck and save under the 'unrestricted' ruleset.**\n";
    deck_import_file << "//**Randomization does not take into account restricted cards, only cards that have been added to Chaotic Recode.**\n";
    deck_import_file << "//**If there is an error with importing this file or generating a random deck, please message me in discord @Sirilean#4526 to report the bug.**\n\n\n";

    //write cards to file
    deck_import_file << "//\tCreatures & Battlegear\n";
    for (int i = 0; i < 6; i++) {
        deck_import_file << "1 " << creatures[i] << "\n";
        deck_import_file << "1 " << battlegear[i] << "\n";
    }

    deck_import_file << "\n//\tMugic\n";
    for (int i = 0; i < 6; i++) {
        deck_import_file << "1 " << mugic[i] << "\n";
    }

    deck_import_file << "\n//\tAttacks\n";
    for (int i = 0; i < 20; i++) {
        deck_import_file << "1 " << attacks[i] << "\n";
    }

    deck_import_file << "\n//\tLocations\n";
    for (int i = 0; i < 10; i++) {
        deck_import_file << "1 " << locations[i] << "\n";
    }

    deck_import_file.close();
    return 0;
}

int main(){
    vector<string> creatures, attacks, battlegear, mugic, locations;

    // open chaotic.json
    Document d = open_doc();

    // build random deck
    creatures = get_creatures(d);
    battlegear = get_battlegear(d, creatures);
    mugic = get_mugic(d, creatures);
    attacks = get_attacks(d);
    locations = get_locations(d);

    // write to file 
    write_file(creatures, attacks, battlegear, mugic, locations);
}
