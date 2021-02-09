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
    # total added: 1079/1225
        # creatures: 434/484
            UW: 100/111
            OW: 111/121
            D: 87/98
            Mip: 93/97
            M'ar: 39/53
            Gen: 4/4
        # battlegear: 126/155
        # attacks: 250/266
        # mugic: 159/189
            UW: 26/32
            OW: 31/34
            D: 28/32
            Mip: 27/33
            M'ar: 10/13
            Gen: 37/45
        # locations: 115/146


    General idea of randomization 
        1) pick random creatures 
            - make first creature have balanced chance of being chosen from any tribe + very small chance of generic (m'arrillians are low chance since there are few of them)
                - **or make every creature selection have the same balancing for tribes
            - if creature is loyal, pick rest of creatures from same tribe. if creature with other tribe from loyal creature already selected, don't add loyal creature
            - if creature is loyal and m'arrillian, allow picking minions as well 
            - if creature is legendary, do not allow for more legendary creatures (hard coded)
            - if card is unique, don't allow multiples  (multiple versions of same card where one is unique and another is not, ex: Headmaster Ankhyja, Headmaster Ankhyja SOTA)
            - don't add more than 2 base versions of a card

       2) pick random attacks (must be chosen after selecting creatures)
            - get most popular element(s) and highest average stat(s) from chosen creatures or most popular stat(s)
                - army must have at least X in an averaged stat to consider adding attacks with that stat's checks/challenges 
                OR
                - at least 3 creatures in the army must have at least X in a stat to consider adding attacks with that stat 
            - only add attacks that contain most popular element(s) of selected creatures (or attacks that have no disciplines/elements)
            - only add attacks that match the highest avg stat(s) (or attacks that have no disciplines/elements)
            - limit the number of high bp attacks able to be added (only allow 2 3bp or higher attacks) 
            - have a minimum of 10 1 bp attacks
            - if card is unique, don't allow multiples 
            - keep track of Build Points (20), keep build point count after adding each attack. if BP is 20 and less than 20 attacks are added, only add attacks with 0 BP, same if BP is 18, only add atacks with 2 BP or less, etc
            - do not add attacks with only 100 stat checks (ex: Academy Strike)
            - check if army loyalty match attack (ex: OW only for Force Balls to be allowed to be added, Danian only for Mandiblor Might) 
                - If Aaune in army, add at least 1 Rage of Aaune

       3) pick random locations 
            - if card is unique, don't allow multiples 
            - don't add more than 2 base versions of a card (Riverlands has 3 versions, etc)
            - If Aaune in army, add at least 1 oligarch's path 

       4) pick random battlegear (must be chosen after selecting creatures)
            - if card is unique, don't allow multiples 
            - if card if legendary, do not add unless chosen creatures match tribe (hard code)
            - don't add more than 2 base versions of a card (Vlaric Shard, Vlaric Shard RoTP)
            - hard code for Baton of Aaune if Aaune is chosen

       5) pick random mugic (must be chosen after selecting creatures)
            - if card is unique, ddon't allow multiples 
            - only add mugic for which there exists a matching tribe from the chosen creatures (and generic) 
            - If Aaune in army, add at least 1 Calling of Aaune

            **Track MC for creatures/mugic and only add mugic that would be able to be played?


    Balancing
        1) The first creature that is selected has an equal chance to be selected from any tribe (19.4%) and a small chance to be generec (4%)
        2) Attacks are selected based on the most popular elements from selected creatures (at least 3 creatures must share an element)
            and the most popular stats from selected creatures (at least 3 creatures must have a minimum of 60 in a shared stat)
        2.1) Attacks that are typically useless are ignored; ex: Slashclaw, Mandiblor Might, 100 stat checks (Glacial Balls, Academy Strike, etc)
        2.2) Attacks that check loyalty are ignored unless selected creatures match it (ex: OW only for Force Balls)
        2.3) The first 10 attacks that are chosen must be 1 BP
        2.4) Do not add more than 2 attacks that are 3 BP or higher 
        3) Do not add mugic from tribes that are not from selected creatures
        4) If Aaune is added, equip it with Baton of Aaune, add 1 Rage of Aaune to attack deck, 1 Calling of Aaune to Mugic, and 1 Oligarch's Path to locations for at least a chance to pull off the flip

    Updates
        1) updated mugic to pick only those from the tribes of selected creatures
        2) updated balance first tribe pick
        3) added check for legendary creatures (generals)
        4) added attack balancing (needs testing)
        5) added hardcoding for Aaune (needs testing)
        6) many attribute additions and errors fixed in chaotic.json
        
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

double get_random(double low, double high) { //gen a random number with different seed every run 
    random_device rd;   
    mt19937 mt(rd());
    uniform_real_distribution<double> dist(low, high); //range is from low to high 
    double random = dist(mt);
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

vector<string> get_tribes(const Document& d, vector<string> creatures) {    //get a vector of all tribes from selected creatures
    vector<string> tribes;

    for (int i = 0; i < 6; i++) {
        Value::ConstMemberIterator tribe_itr = d["creatures"][creatures[i].c_str()].FindMember("tribe");
        string tribe = tribe_itr->value.GetString();
        bool included = false;
        for (auto t : tribes) {
            if (t == tribe)
                included = true;
        }
        if (!included)
            tribes.push_back(tribe);
    }
    //print tribes 
    std::cout << "\n\tTRIBES" << endl;
    int c = 1;
    for (auto t : tribes)
        std::cout << "\t" << c++ << ": " << t << endl;
    return tribes; 
}

string pick_tribe() { //randomly pick a tribe to use for the first random creature to give fairer chance for each tribe to be chosen
    //19.4% chance for each tribe + 4% for generic = 100%
    double random = get_random(0.0, 100.0);
    std::cout << random << endl;

    if (random <= 19.4) return "overworld";
    if (random > 19.4 && random <= 38.8) return "underworld";
    if (random > 38.8 && random <= 58.2) return "danian";
    if (random > 58.2 && random <= 77.6) return "mipedian";
    if (random > 77.6 && random <= 97) return "m'arrillian";
    return "generic";
}

bool check_vector(vector<string> vector, string word) { // check if word is in vector 
    for (auto s : vector) {
        if (s == word)
            return true;
    }
    return false;
}

vector<string> get_army_elements(const Document& d, vector<string> creatures) { //get most army's most popular element(s) (minimum 3 creatures must have the same element, otherwise, return none)
    vector<string> elements;
    vector<int> elementCount = { 0, 0, 0, 0 }; //index = elem: 0f, 1a, 2e, 3w 

    for (int i = 0; i < creatures.size()-1; i++) {  //add counts of creature's elements 
        //std::cout << creatures[i] << endl;
        Value::ConstMemberIterator card_itr = d["creatures"][creatures[i].c_str()].FindMember("elements");
        string c_elements = card_itr->value.GetString();
        if (!c_elements.empty()) {
            stringstream ss(card_itr->value.GetString());
            //std::cout << card_itr->value.GetString() << endl;
            while (ss.good()) { // parse the elements string 
                string substr;
                getline(ss, substr, ',');
                if (substr == "fire")
                    elementCount[0]++;
                if (substr == "air")
                    elementCount[1]++;
                if (substr == "earth")
                    elementCount[2]++;
                if (substr == "water")
                    elementCount[3]++;
            }
        }
    }

    //printf("F: %d, A: %d, E: %d, W: %d\n", elementCount[0], elementCount[1], elementCount[2], elementCount[3]);
    int max = *max_element(elementCount.begin(), elementCount.end());   //get most popular element

    if (max >= 3) { //if at least 3 creatures share the same element 
        for (int i = 0; i < elementCount.size(); i++) { // for each element that is >= to 3, add that to final elements vector
            if (elementCount[i] >= 3) {
                if (i == 0)
                    elements.push_back("fire");
                if (i == 1)
                    elements.push_back("air");
                if (i == 2)
                    elements.push_back("earth");
                if (i == 3)
                    elements.push_back("water");
            }
        }
    }

    std::cout << "\tPOPULAR ELEMENTS" << endl;
    for (auto e : elements)
        std::cout << e << endl;
    return elements; 
}

vector<string> get_army_stats_avg(const Document& d, vector<string> creatures) {    //return vect of stats from army's avg that meet the threshold
    vector<string> stats; 
    int statThreshold = 50;
    int sumCourage = 0;
    int sumPower = 0;
    int sumWisdom = 0;
    int sumSpeed = 0;

    for (int i = 0; i < creatures.size() - 1; i++) {  //add all stats from each creature 
        //std::cout << creatures[i] << endl;
        int count = 0; 
        for (auto itr = d["creatures"][creatures[i].c_str()]["disciplines"].MemberBegin(); itr != d["creatures"][creatures[i].c_str()]["disciplines"].MemberEnd(); ++itr) {
            int value = itr->value.GetInt();
            //std::cout << "\t" << value << endl;
            if (count == 0)
                sumCourage += value;
            if (count == 1)
                sumPower += value;
            if (count == 2)
                sumWisdom += value;
            if (count == 3)
                sumSpeed += value;
            count++;
        }
        //printf("\nSUM:\tC: %d, P: %d, W: %d, S: %d\n", sumCourage, sumPower, sumWisdom, sumSpeed);
    }

    // divide by # of creatures for avg
    sumCourage /= creatures.size();
    sumPower /= creatures.size();
    sumWisdom /= creatures.size();
    sumSpeed /= creatures.size();
    //printf("\nAVG:\tC: %d, P: %d, W: %d, S: %d\n", sumCourage, sumPower, sumWisdom, sumSpeed);

    if (sumCourage >= statThreshold)
        stats.push_back("courage");
    if (sumPower >= statThreshold)
        stats.push_back("power");
    if (sumWisdom >= statThreshold)
        stats.push_back("wisdom");
    if (sumSpeed >= statThreshold)
        stats.push_back("speed");

    std::cout << "\tAVG STATS" << endl;
    for (auto s : stats)
        std::cout << s << endl;
    return stats; 
}

vector<string> get_army_stats_indv(const Document& d, vector<string> creatures) {    //return vect of most popular stats where at least x creatures meet the threshold for a stat
    vector<string> stats;
    int statThreshold = 60; 
    int countCourage = 0;
    int countPower = 0;
    int countWisdom = 0;
    int countSpeed = 0;

    for (int i = 0; i < creatures.size() - 1; i++) {  //add all stats from each creature 
        //std::cout << creatures[i] << endl;
        int count = 0;
        for (auto itr = d["creatures"][creatures[i].c_str()]["disciplines"].MemberBegin(); itr != d["creatures"][creatures[i].c_str()]["disciplines"].MemberEnd(); ++itr) {
            int value = itr->value.GetInt();
            //std::cout << "\t" << value << endl;
            if (count == 0 && value >= statThreshold)
                countCourage++;
            if (count == 1 && value >= statThreshold)
                countPower++;
            if (count == 2 && value >= statThreshold)
                countWisdom++;
            if (count == 3 && value >= statThreshold)
                countSpeed++;
            count++;
        }
        //printf("\nCOUNT:\tC: %d, P: %d, W: %d, S: %d\n", countCourage, countPower, countWisdom, countSpeed);
    }

    //for any stat that has at least 3 creatures with 50 in that stat, add it 
    if (countCourage >= 3)
        stats.push_back("courage");
    if (countPower >= 3)
        stats.push_back("power");
    if (countWisdom >= 3)
        stats.push_back("wisdom");
    if (countSpeed >= 3)
        stats.push_back("speed");

    std::cout << "\tINDV STATS" << endl;
    for (auto s : stats)
        std::cout << s << endl;
    return stats;
}

bool check_attack_elements(const Document& d, vector<string> elements, string attack) {  //read a card's elements and return a vector of them 
    bool matches_elements = false;

    Value::ConstMemberIterator card_itr = d["attacks"][attack.c_str()].FindMember("elements");
    //std::cout << card_itr->value.GetString() << endl;

    stringstream ss(card_itr->value.GetString());
    //std::cout << card_itr->value.GetString() << endl;
    while (ss.good()) {
        string substr;
        getline(ss, substr, ',');
        if (substr == "f" and check_vector(elements, "fire")) //check if attack matches army elements 
            return true;
        else if (substr == "a" and check_vector(elements, "air"))
            return true;
        else if (substr == "e" and check_vector(elements, "earth"))
            return true;
        else if (substr == "w" and check_vector(elements, "water"))
            return true;
    }

    return false;
}

bool check_attack_stats(const Document& d, vector<string> disciplines, string attack) {  //check if attack matches army avg stats 
    Value::ConstMemberIterator card_itr = d["attacks"][attack.c_str()].FindMember("elements");
    //std::cout << card_itr->value.GetString() << endl;

    stringstream ss(card_itr->value.GetString());
    //std::cout << card_itr->value.GetString() << endl;
    while (ss.good()) {
        string substr;
        getline(ss, substr, ',');
        if (substr == "c" and check_vector(disciplines, "courage"))
            return true;
        else if (substr == "p" and check_vector(disciplines, "power"))
            return true;
        else if (substr == "w" and check_vector(disciplines, "wisdom"))
            return true;
        else if (substr == "s" and check_vector(disciplines, "speed"))
            return true;
    }

    return false;
}

vector<string> get_locations(const Document& d, vector<string> creatures) {
    int loc_count = 0;
    vector<string> locations;   //vector of added locations 

    //check for Aaune, this is optional
    if (check_vector(creatures, "Aa'une the Oligarch, Projection")) {
        locations.push_back("The Oligarch's Path");
        loc_count++;
    }

    while (loc_count < 10) {    // until 10 locations are picked 
        int random = get_random(0, 145); // gen a random number from 0 to 145 (146 total location cards)
        //std::cout << "Random: " << random << endl;

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
                std::cout << "\tLOCATION: more than one copy of card and at least one is Unique" << endl;
                dont_add = true;
                break;
            }
            duplicates++; //increase # of copies of current card in locations
        }
    }

    if (duplicates <= 1 && dont_add == false) {   //if there is at most 1 duplicate already in locations, add card
        //std::cout << "\tLOCATION: added" << endl;
        locations.push_back(card);
        loc_count++;
    }

    else if (duplicates >= 2) {  //else there are already 2 copies of the card, so dont add
        //std::cout << "\tLOCATION: 2 copies aleady" << endl;
        dont_add = true;
        break;
    }
}

else {  //current card is not added in recode
    //std::cout << "\tLOCATION: not in recode" << endl;
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
    std::cout << "\n\tLOCATIONS" << endl;
    int c = 1;
    for (auto l : locations)
        std::cout << "\t" << c++ << ": " << l << endl;
    return locations;
}

vector<string> get_attacks(const Document& d, vector<string> creatures) { //there is a small chance that the 20 attacks are below the 20 bp limit, not sure if to leave it or make it so it always reaches 20
    int total_bp = 0;
    int attack_count = 0;
    int highBP_count = 0; //max 2 high bp attacks (3 bp or more)
    int oneBP_count = 0; //at least 10 1 bp attacks 
    vector<string> army_elements = get_army_elements(d, creatures);
    vector<string> avg_stats = get_army_stats_avg(d, creatures);
    vector<string> indv_stats = get_army_stats_indv(d, creatures);
    vector<string> attacks;

    //check for Aaune, this is optional
    if (check_vector(creatures, "Aa'une the Oligarch, Projection")) {
        attacks.push_back("Rage of Aa'une");
        oneBP_count++;
        attack_count++;
        total_bp += 1;
    }

    cout << "LOYALTY " << creatures[6] << endl;
    while (attack_count < 20) {    // until 20 attacks are picked 
        int random = get_random(0, 265); // gen a random number from 0 to 265 (266 total attacks)
        //std::cout << "Random: " << random << endl;

        //iterate through cards until you reach random card
        int count = 0;
        for (auto itr = d["attacks"].MemberBegin(); itr != d["attacks"].MemberEnd(); ++itr) {
            bool dont_add = false; //if there is some problem with adding a card (2 copies/unique), trigger this to stop the loop and gen a new random number

            if (count == random) { //stop when the random card is reached
                //get card attribs 
                string card, added, unique, elements, disciplines;
                int bp;
                card = itr->name.GetString(); //full card name 
                Value::ConstMemberIterator card_itr = d["attacks"][card.c_str()].FindMember("added");
                added = card_itr->value.GetString(); //is card in recode 
                card_itr = d["attacks"][card.c_str()].FindMember("unique");
                unique = card_itr->value.GetString(); //is card unique 
                card_itr = d["attacks"][card.c_str()].FindMember("bp");
                bp = stoi(card_itr->value.GetString()); //get attack's build point cost
                card_itr = d["attacks"][card.c_str()].FindMember("elements");
                elements = card_itr->value.GetString(); //get attacks elements, if any
                card_itr = d["attacks"][card.c_str()].FindMember("disciplines");
                disciplines = card_itr->value.GetString(); ///get attacks stats, if any
                //std::cout << card << ", " << bp << endl;


                //check if army loyalty matches attack (ex: OW loyalty needed in order to add Force Balls)
                if (card == "Force Balls") {
                    if (creatures[6] != "overword") {
                        dont_add = true;
                        break;
                    }
                }
                if (card == "Mightswing") {
                    if (creatures[6] != "underworld") {
                        dont_add = true;
                        break;
                    }
                }
                if (card == "Sandstrike") {
                    if (creatures[6] != "mipedian") {
                        dont_add = true;
                        break;
                    }
                }
                if (card == "Swarming Destruction") {
                    if (creatures[6] != "danian") {
                        dont_add = true;
                        break;
                    }
                }

                // only add 10 1 bps attacks as the first 10 attacks
                if (attack_count < 10 && oneBP_count < 10) {
                    if (bp != 1) {
                        //std::cout << "\tATTACK: must add 10 1 bp attacks first " << endl;
                        dont_add = true;
                        break;
                    }
                }

                //if bp > 2 and already have 2 high bp attacks, dont add 
                if (highBP_count == 2 && bp > 2) {
                    //std::cout << "\tATTACK: over high BP limit" << endl;
                    dont_add = true;
                    break;
                }

                if (added == "1") {   //if current card is in recode and adding it won't go over 20 build points, try to add card to attacks 
                    if (total_bp + bp > 20) { //if adding card goes over 20 bp, dont add
                        //std::cout << "\tATTACK: over 20 bp" << endl;
                        dont_add = true;
                        break;
                    }

                    // check attack matches elements/stats 
                    if (army_elements.size() > 0 && !elements.empty()) { //if attack has elements, check it matches the popular elements of army (if the army has any)
                        if (!check_attack_elements(d, army_elements, card)) {
                            //std::cout << "\tATTACK: doesn't match elements" << endl;
                            dont_add = true;
                            break;
                        }
                    }
                    if (avg_stats.size() > 0 && !disciplines.empty()) { //if attack has any stat check/challenge, check it matches the avg stats of the army (if the army has any)
                        if (!check_attack_stats(d, avg_stats, card)) {
                            //std::cout << "\tATTACK: doesn't match stats" << endl;
                            dont_add = true;
                            break;
                        }
                    }

                    ////if attack has any stat check/challenge, check it matches the individual stats of the army (if the army has any)
                    //if (indv_stats.size() > 0 &&  !disciplines.empty()) { 
                    //    if (!check_attack_stats(d, indv_stats, card)) {
                    //        //std::cout << "\tATTACK: doesn't match stats" << endl;
                    //        dont_add = true;
                    //        break;
                    //    }
                    //}

                    //if not popular elements/stats in army, only add attacks w/o elements/stats 
                    if (indv_stats.size() == 0 && !disciplines.empty()) {
                        dont_add = true;
                        break;
                    }
                    if (army_elements.size() == 0 && !elements.empty()) {
                        dont_add = true;
                        break;
                    }

                    int duplicates = 0; //count for tracking copies of current card already in locations 

                    //check for more than one copy of card and if at least one is Unique
                    for (string atk : attacks) {

                        if (atk == card) {   //if card with same name in attacks, check if that card is unique 
                            if (unique == "1") { //if current card is unique, dont add current card 
                                //std::cout << "\tATTACK: more than one copy of card and at least one is Unique" << endl;
                                dont_add = true;
                                break;
                            }
                            duplicates++; //increase # of copies of current card in attacks
                        }
                    }

                    if (duplicates <= 1 && dont_add == false) {   //if there is at most 1 duplicate already in attack, add card and inc total_bp
                        //std::cout << "\tATTACK: added" << endl;
                        attacks.push_back(card);
                        attack_count++;
                        total_bp += bp;
                        //std::cout << total_bp << endl;

                        //update 1 bp count
                        if (bp == 1) oneBP_count++;
                        //update high bp count
                        if (bp >= 3) highBP_count++;
                    }

                    if (duplicates >= 2) {  //else there are already 2 copies of the card, so dont add
                        //std::cout << "\tATTACK: 2 copies aleady" << endl;
                        dont_add = true;
                        break;
                    }
                }

                else {  //current card is not added in recode
                    //std::cout << "\tATTACK: not in recode" << endl;
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
    std::cout << "\n\tATTACKS" << endl;
    int c = 1;
    for (auto a : attacks)
        std::cout << "\t" << c++ << ": " << a << endl;
    std::cout << "\t Total BP: " << total_bp << endl;
    return attacks;
}

vector<string> get_creatures(const Document& d) {
    vector<string> creatures;   //track added creatures
    int creature_count = 0;
    string loyalty = ""; //if the first creature picked is loyal to a tribe, set this to that tribe and only choose creatures from that tribe (or minions if m'arrillian)
    string first_tribe = pick_tribe();
    cout << "First Tribe: " << first_tribe << endl;

    while (creature_count < 6) {    // until 6 creatures are picked 
        int random = get_random(0, 483); // gen a random number from 0 to 483 (484 total creatures)
        //std::cout << "Random: " << random << endl;

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
                //std::cout << card << endl;

                //cout << "TRIBE: " << tribe << ", FIRST TRIBE: " << first_tribe << endl;
                //if the first chosen creature doesn't match the initial tribe choice, dont add
                if (creature_count == 0 && tribe.compare(first_tribe) != 0) {
                    //cout << "\tCREATURES: first tribe mismatch" << endl;
                    dont_add = true;
                    break;
                }

                if (added == "1") {   //if current card is in recode and adding it won't go over 20 build points, try to add card to attacks

                    //check for the 4 generals, make sure no other generals are in army before adding 
                    if (card == "Gorram, Danian General" 
                        && !(check_vector(creatures, "Barath Beyond, UnderWorld General") || check_vector(creatures, "Tangath Toborn, OverWorld General") || check_vector(creatures, "Grantkae, Mipedian General"))) {
                        dont_add = true;
                        break;
                    }
                    if (card == "Barath Beyond, UnderWorld General"
                        && !(check_vector(creatures, "Gorram, Danian General") || check_vector(creatures, "Tangath Toborn, OverWorld General") || check_vector(creatures, "Grantkae, Mipedian General"))) {
                        dont_add = true;
                        break;
                    }
                    if (card == "Tangath Toborn, OverWorld General"
                        && !(check_vector(creatures, "Barath Beyond, UnderWorld General") || check_vector(creatures, "Gorram, Danian General") || check_vector(creatures, "Grantkae, Mipedian General"))) {
                        dont_add = true;
                        break;
                    }
                    if (card == "Grantkae, Mipedian General"
                        && !(check_vector(creatures, "Barath Beyond, UnderWorld General") || check_vector(creatures, "Tangath Toborn, OverWorld General") || check_vector(creatures, "Gorram, Danian General"))) {
                        dont_add = true;
                        break;
                    }

                    if (loyal != "") { //if current card is loyal
                        //std::cout << "LOYAL" << endl;
                        if (creature_count == 0) { //if this is the first card, set loyalty for rest of creatures 
                            //std::cout << "\tCREATURE: first creature loyal to " << loyal << endl;
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
                                    //std::cout << "\tCREATURE: 1prev creature loyal to m'arrillian and curr creature not m'arrillian or minion" << endl;
                                    dont_add = true;
                                    break;
                                }
                                else if (added_creature_loyal != "m'arrillian" && added_creature_loyal != tribe) { //else if prev added creature was another tribe that doesn't match tribe of current card, don't add 
                                    //std::cout << "\tCREATURE: 1prev creature is " << added_creature_tribe << " and cur creature is loyal to " << loyal << endl;
                                    dont_add = true;
                                    break;
                                }
                            }
                            else {  //else prev creature is not loyal 
                                //if a tribe from added creatures doesn't match current creature, dont add 
                                if (added_creature_tribe == "m'arrillian" && (tribe != "m'arrillian" && minion != "1")) {  //if prev added creature is m'ar and current creature is not m'ar or minion, dont add
                                    //std::cout << "\tCREATURE: 2prev creature m'arrillian and curr loyal creature not m'arrillian or minion" << endl;
                                    dont_add = true;
                                    break;
                                }
                                else if (added_creature_tribe != tribe) { //else if prev added creature was another tribe that doesn't match loyalty of current card, don't add 
                                    //std::cout << "\tCREATURE: 2prev creature is " << added_creature_tribe << " and cur creature is loyal to " << loyal << endl;
                                    dont_add = true;
                                    break;
                                }
                            }
                        }
                    }
                    else { //if current card not loyal, check if any prev added cards are loyal and if cur card matches tribe
                        //std::cout << "NOT LOYAL" << endl;
                        for (auto cr : creatures) { //check if any other creatures added before would disallow the current card b/c of loyalty
                            card_itr = d["creatures"][cr.c_str()].FindMember("tribe");  //get added creature's tribe, loyalty, and check if its a minion
                            string added_creature_tribe = card_itr->value.GetString();
                            card_itr = d["creatures"][cr.c_str()].FindMember("minion");
                            string added_creature_minion = card_itr->value.GetString();
                            card_itr = d["creatures"][cr.c_str()].FindMember("loyal");
                            string added_creature_loyal = card_itr->value.GetString();

                            if (added_creature_loyal != "") { //if prev added creature is loyal
                                if (added_creature_loyal == "m'arrillian" && (tribe != "m'arrillian" && minion != "1")) { //if prev added creature is loyal to m'ar and current creature is not m'ar or minion, dont add
                                    //std::cout << "\tCREATURE: 3prev creature loyal to m'arrillian and curr creature not m'arrillian or minion" << endl;
                                    dont_add = true;
                                    break;
                                }
                                else if (added_creature_loyal != "m'arrillian" && added_creature_loyal != tribe) { //else if prev added creature was another tribe that doesn't match tribe of current card, don't add 
                                    //std::cout << "\tCREATURE: 3prev creature is " << added_creature_tribe << " and cur creature is loyal to " << loyal << endl;
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
                                //std::cout << "\tCREATURE: more than one copy of card and at least one is Unique" << endl;
                                dont_add = true;
                                break;
                            }
                            duplicates++; //increase # of copies of current card in creatures
                        }
                    }

                    if (duplicates <= 1 && dont_add == false) {   //if there is at most 1 duplicate already in creatures, add card 
                        //std::cout << "\tCREATURE: added" << endl;
                        creatures.push_back(card);
                        creature_count++;
                    }

                    if (duplicates >= 2) {  //else there are already 2 copies of the card, so dont add
                        //std::cout << "\tCREATURE: 2 copies aleady" << endl;
                        dont_add = true;
                        break;
                    }
                }

                else {  //current card is not added in recode
                    //std::cout << "\tCREATURE: not in recode" << endl;
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
    std::cout << "\n\tCREATURES" << endl;
    for (int c = 1; c < 7; c++)
        std::cout << "\t" << c << ": " << creatures[c-1] << endl;
    return creatures;
}

vector<string> get_battlegear(const Document& d, vector<string> creatures) {
    vector<string> battlegear;
    int bg_count = 0;
    string loyalty = ""; 
    //if (creatures[6] != "") { //if there is loyalty for the selected creatures, set loyalty for possible legendary battlegear 
    loyalty = creatures[6];
    //std::cout << "\tBATTLEGEAR: loyalty = " << loyalty << endl;
    //}

    //check creatures for Aaune and get it's index 
    int aauneIndex = -1;
    for (int i = 0; i < 6; i++) {
        if (creatures[i] == "Aa'une the Oligarch, Projection") {
            aauneIndex = i;
            break;
        }
    }

    while (bg_count < 6) {    // until 6 bg are picked 
        //if Aaune in army and bg_count == its index, give it Baton of Aaune
        if (bg_count == aauneIndex) {
            battlegear.push_back("Baton of Aa'une");
            bg_count++;
        }

        int random = get_random(0, 154); // gen a random number from 0 to 154 (155 total bg)
        //std::cout << "Random: " << random << endl;

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
                //std::cout << card << endl;

                if (added == "1") {   //if current card is in recode and adding it won't go over 20 build points, try to add card to attacks

                    //hard code check for legendary bg 
                    if (card == "Crown of Aa'une" && loyalty != "m'arrillian") {    //if loyalty from selected creatures doesn't match the leg bg, don't add 
                        dont_add = true;
                        break;
                    }
                    if (card == "Dread Tread" && loyalty != "underworld") {    
                        dont_add = true;
                        break;
                    }                    
                    if (card == "Hornsabre" && loyalty != "overworld") {
                        dont_add = true;
                        break;
                    }
                    if (card == "Scepter of the Infernal Parasite" && loyalty != "danian") {
                        dont_add = true;
                        break;
                    }
                    if (card == "Warriors of Owayki" && loyalty != "mipedian") {
                        dont_add = true;
                        break;
                    }
   
                    int duplicates = 0; //count for tracking copies of current card already in battlegear 

                    //check for more than one copy of card and if at least one is Unique
                    for (string bg : battlegear) {
                        string bg_parsed = parse_name(bg);

                        if (bg_parsed == card_parsed) {   //if card with same base name in battlegear, check if that card is unique 
                            Value::ConstMemberIterator check_unique = d["battlegear"][bg.c_str()].FindMember("unique");
                            string bg_unique = check_unique->value.GetString();

                            if (bg_unique == "1" || unique == "1") { //if card in battlegear is unique or current card is unique, dont add current card 
                                //std::cout << "\tBATTLEGEAR: more than one copy of card and at least one is Unique" << endl;
                                dont_add = true;
                                break;
                            }
                            duplicates++; //increase # of copies of current card in battlegear
                        }
                    }

                    if (duplicates <= 1 && dont_add == false) {   //if there is at most 1 duplicate already in battlegear, add card 
                        //std::cout << "\tBATTLEGEAR: added" << endl;
                        battlegear.push_back(card);
                        bg_count++;
                    }

                    else if (duplicates >= 2) {  //else there are already 2 copies of the card, so dont add
                        //std::cout << "\tBATTLEGEAR: 2 copies aleady" << endl;
                        dont_add = true;
                        break;
                    }
                }

                else {  //current card is not added in recode
                    //std::cout << "\tBATTLEGEAR: not in recode" << endl;
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
    std::cout << "\n\tBATTLEGEAR" << endl;
    int c = 1;
    for (auto bg : battlegear)
        std::cout << "\t" << c++ << ": " << bg << endl;
    return battlegear;
}

vector<string> get_mugic(const Document& d, vector<string> creatures) { //mugic is only selected if it is generic or part of the tribes from the chosen creatures
    vector<string> mugic;
    vector<string> tribes = get_tribes(d, creatures);
    int mugic_count = 0;

    //check for Aaune, this is optional
    if (check_vector(creatures, "Aa'une the Oligarch, Projection")) {
        mugic.push_back("Calling of Aa'une");
        mugic_count++;
    }

    while (mugic_count < 6) {    // until 6 mugic are picked 
        int random = get_random(0, 188); // gen a random number from 0 to 188 (189 total mugic)

        //iterate through cards until you reach random card
        int count = 0;
        for (auto itr = d["mugic"].MemberBegin(); itr != d["mugic"].MemberEnd(); ++itr) {
            bool dont_add = false; //if there is some problem with adding a card (2 copies/unique), trigger this to stop the loop and gen a new random number

            if (count == random) { //stop when the random card is reached

                //get card attribs 
                string card, card_parsed, added, unique, tribe;
                card = itr->name.GetString(); //full card name 
                card_parsed = parse_name(card); //parsed name 
                Value::ConstMemberIterator card_itr = d["mugic"][itr->name.GetString()].FindMember("added");
                added = card_itr->value.GetString(); //is card in recode 
                card_itr = d["mugic"][itr->name.GetString()].FindMember("unique");
                unique = card_itr->value.GetString(); //is card unique 
                card_itr = d["mugic"][itr->name.GetString()].FindMember("tribe");
                tribe = card_itr->value.GetString(); //is card unique 
                //std::cout << card << endl;

                //check mugic is one of the tribes from chosen creatures
                bool is_tribe = false;
                for (auto t : tribes) {
                    if (t == tribe) {
                        is_tribe = true;
                        break;
                    }
                }
                if (!is_tribe) {
                    //std::cout << "\tMUGIC: mugic doesn't match tribes" << endl;
                    dont_add = true;
                    break;
                }

                if (added == "1") {   //if current card is in recode and adding it won't go over 20 build points, try to add card to attacks
                    int duplicates = 0; //count for tracking copies of current card already in battlegear 

                    //check for more than one copy of card and if at least one is Unique
                    for (string m : mugic) {
                        string m_parsed = parse_name(m);

                        if (m_parsed == card_parsed) {   //if card with same base name in battlegear, check if that card is unique 
                            Value::ConstMemberIterator check_unique = d["mugic"][m.c_str()].FindMember("unique");
                            string m_unique = check_unique->value.GetString();

                            if (m_unique == "1" || unique == "1") { //if card in battlegear is unique or current card is unique, dont add current card 
                                //std::cout << "\tMUGIC: more than one copy of card and at least one is Unique" << endl;
                                dont_add = true;
                                break;
                            }
                            duplicates++; //increase # of copies of current card in battlegear
                        }
                    }

                    if (duplicates <= 1 && dont_add == false) {   //if there is at most 1 duplicate already in battlegear, add card 
                        //std::cout << "\tMUGIC: added" << endl;
                        mugic.push_back(card);
                        mugic_count++;
                    }

                    else if (duplicates >= 2) {  //else there are already 2 copies of the card, so dont add
                        //std::cout << "\tMUGIC: 2 copies aleady" << endl;
                        dont_add = true;
                        break;
                    }
                }

                else {  //current card is not added in recode
                    //std::cout << "\tMUGIC: not in recode" << endl;
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
    std::cout << "\n\tMUGIC" << endl;
    int c = 1;
    for (auto m : mugic)
        std::cout << "\t" << c++ << ": " << m << endl;
    return mugic;
}


int main(){
    vector<string> creatures, attacks, battlegear, mugic, locations, tribes;

    // open chaotic.json
    Document d = open_doc();

    // build random deck
    creatures = get_creatures(d);   
    battlegear = get_battlegear(d, creatures);
    mugic = get_mugic(d, creatures); 
    attacks = get_attacks(d, creatures); 
    locations = get_locations(d, creatures);

    // write to file 
    write_file(creatures, attacks, battlegear, mugic, locations);
}
