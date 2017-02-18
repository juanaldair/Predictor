//============================================================================
// Name        : helloworld.cpp
// Author      : fernando
// Version     :
// Copyright   : Your copyright notice
// Description : Text predictor projector
//============================================================================


/*-----------------------------------------------------------------------------------*/
/*                                   I N C L U D E                                   */
/*-----------------------------------------------------------------------------------*/
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>

// Concurrency
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <condition_variable>


/*-----------------------------------------------------------------------------------*/
/*                                   D E F I N E S                                   */
/*-----------------------------------------------------------------------------------*/
//#define MAX_WORD_LENGTH 10


/*-----------------------------------------------------------------------------------*/
/*                                    M A C R O S                                    */
/*-----------------------------------------------------------------------------------*/
#define MEASURE_EXE_TIME_BEGIN() t1 = std::chrono::high_resolution_clock::now()
#define MEASURE_EXE_TIME_END();  t2 = std::chrono::high_resolution_clock::now();\
								 timeDiff = std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count();



/*-----------------------------------------------------------------------------------*/
/*                                     E N U M S                                     */
/*-----------------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------------*/
/*                                  T Y P E D E F S                                  */
/*-----------------------------------------------------------------------------------*/
typedef std::vector<std::string>::iterator wd_iterator;


/*-----------------------------------------------------------------------------------*/
/*                        P R O T O T Y P E   F U N C T I O N                        */
/*-----------------------------------------------------------------------------------*/
void UserInterfaceThread(void);
void TextProcessorThread(std::string filename);

std::vector<int> readDictionaryFromFile(std::string filename, std::vector<std::string> &dic);
bool search(wd_iterator &begin, wd_iterator &end, char letter, int &letterPos);


/*-----------------------------------------------------------------------------------*/
/*                               G L O B A L   V A R S                               */
/*-----------------------------------------------------------------------------------*/
std::mutex m, synchMtx;
std::condition_variable synch;
std::string word;
bool dataReady = false;

/*-----------------------------------------------------------------------------------*/
/*                              M A I N   P R O G R A M                              */
/*-----------------------------------------------------------------------------------*/
int main(int argc, char *argv[]) {
	// Local variables
	std::thread TPThread, ReadDictionaryThread, UIThread; // Threads for multithread
	std::string filename = "dictionary.txt";

	// Start threads
	UIThread = std::thread(UserInterfaceThread);
	TPThread = std::thread(TextProcessorThread, filename);

	while( true ) {
		// To exit: write "exit" word
		m.lock();
		int flag = word.compare("exit");
		m.unlock();

		if(  flag == 0 ) break;
		else std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	m.lock();
	std::cout << "Exiting..." << std::endl;
	m.unlock();

	if( UIThread.joinable() ) UIThread.detach();
	if( TPThread.joinable() ) TPThread.detach();

	return 0;
}


/*-----------------------------------------------------------------------------------*/
/*                        R E F E R E N C E   F U N C T I O N                        */
/*-----------------------------------------------------------------------------------*/
/*
 * @name:	UserInterfaceThread
 * @brief:	This function read data from user and concatenate each data in a string.
 */
void UserInterfaceThread(void) {

	while( true ) {
		char input = getchar();

		m.lock();
		if( input != '\n' ) {
			if(input == VK_SPACE) word.clear();
			else word += input;

			std::unique_lock<std::mutex> lk(synchMtx);
			dataReady = true;
			synch.notify_one();
		} else std::cout << "String: " << word << std::endl;
		m.unlock();
	}
}


/*
 * @name:	TextProcessorThread
 * @brief:	This function read data from user and concatenate each data in a string.
 */
void TextProcessorThread(std::string filename) {
	// Local parameters
	std::vector<std::string> dictionary;	// The dictionary
	std::vector<int> offsets;
	char letter;
	int letterOffset = 0;
	wd_iterator begin, end;
	bool running = true;

	// Configurations
	offsets = readDictionaryFromFile(filename, dictionary);

	// Main program for text processor thread
	while( true ) {
		// Wait for new data
		std::unique_lock<std::mutex> lck(synchMtx);
		while( !dataReady ) synch.wait(lck);
		m.lock();
		dataReady = false;
		letter = word.back();
		m.unlock();

		// Processing data
		if( letterOffset == 0 ) {
			int idx = int(letter - 'a');
			begin = dictionary.begin() + offsets.at(idx++);
			end   = (letter == 'z' ? dictionary.end() : dictionary.begin() + offsets.at(idx));
			letterOffset++;
		} else {
			if( letter == 0 ) {
				letterOffset = 0;
				running = true;
			}
			else if( running ) running = search(begin, end, letter, letterOffset);
		}

		std::this_thread::yield();
	}
}

/*
 * @name:	readDictionaryFromFile
 * @brief:	This function read a dictionary from file and saves it in a vector of
 * 			strings.
 * @return:	offset indexes.
 */
std::vector<int> readDictionaryFromFile(std::string filename, std::vector<std::string> &dic) {
	// Local variables
	std::ifstream dictionaryFile(filename);
	std::string line;
	std::vector<int> indexes;
	long long cnt = 0;
	char letter = 'a';

	if( !dictionaryFile.is_open() ) return indexes;

	// Get each word inside the dictionary file
	while( std::getline(dictionaryFile, line) ) {
		dic.push_back(line); // Push each word in a vector

		if( letter <= 'z' && line.front() == letter ) {
			letter++;
			indexes.push_back( cnt );
		}

		cnt++;
	}

	dictionaryFile.close();

	return indexes;
}

bool search(wd_iterator &begin, wd_iterator &end, char letter, int &letterPos) {
	bool status = false;

	for(auto it = begin; it != end; ++it) {
		char character = it->at(letterPos);

		if( it->length() >= (size_t)letterPos ) {
			if( !status ) {
				if( character == letter ) {
					begin = it;
					letter++;
					status = true;
				}
			} else {
				if( character == letter || character != letter - 1) {
					end = it;
					letterPos++;
					break;
				}
			} // end if-else
		} // end if
	} // End for

	return status;
}

/* END OF FILE: demo.cpp ------------------------------------------------------------*/
