/**
 * @file example.cc
 * @brief Add description here
 */

#include "person.pb.h"
#include "../recordio.h"

#include <iostream>
#include <string>


int main() {
  // Write
  std::ofstream ofs("example.recordio", std::ios::binary);
  recordio::RecordWriter writer(&ofs);

  std::string names[] = {"Smith", "Johnson", "William", "Jones", "Brown"};
  int age = 20;
  for (const auto& name : names) {
    Person person;
    person.set_name(name);
    person.set_age(age++);
    writer.WriteProtocolMessage(person);
  }
  writer.Close();

  // Read
  std::ifstream ifs("example.recordio", std::ios::binary);
  recordio::RecordReader reader(&ifs);

  Person person;
  while(reader.ReadProtocolMessage(&person)) {
    std::cout << person.name() << " (" << person.age() << ")" << std::endl;
  }

  return 0;
}
