#include <cart.h>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

char cur_path[4096];
uint8_t cartridge_data[MAX_CART_SIZE];
bool cartridge_loaded = false;
cart_header_struct *cartridge_header =
    (cart_header_struct *)(cartridge_data + 0x100);

bool get_runtime_path() {
  return getcwd(cur_path, sizeof(cur_path)) != nullptr;
}

bool cart_open_file() {
  if (!get_runtime_path()) {
    fprintf(stderr, "Failed to get runtime path\n");
    return false;
  }

  char filename[4096];
  // make the user type in the path to the rom file relative from runtime path
  printf("Enter the path to the rom file relative from runtime path: ");
  std::cin.getline(filename, sizeof(filename));

  std::string rom_path = std::string(cur_path) + "/" + filename;
  // if we could open the file, load it into cartridge_data
  FILE *rom_file = fopen(rom_path.c_str(), "rb");
  if (rom_file) {
    cart_load(rom_path.c_str());
    return true;
  }
  return false;
}

void cart_print_info() {
  printf("Entry point: %.2X%.2X%.2X%.2X\n", cartridge_header->entry_point[0],
         cartridge_header->entry_point[1], cartridge_header->entry_point[2],
         cartridge_header->entry_point[3]);
  printf("Title: %s\n", cartridge_header->title);
  printf("CBG flag: %.2X\n", cartridge_header->cgb_flag);
  printf("New Licensee Code: %.2X%.2X\n",
         cartridge_header->new_licensee_code[0],
         cartridge_header->new_licensee_code[1]);
  printf("SBG flag: %.2X\n", cartridge_header->sbg_flag);
  printf("Cartridge Type: %.2X\n", cartridge_header->cgb_flag);
  printf("ROM size: %.2X\n", cartridge_header->rom_size);
  printf("RAM size: %.2X\n", cartridge_header->ram_size);
  printf("Destination Code: %.2X\n", cartridge_header->destination_code);
  printf("Old Licensee Code: %.2X\n", cartridge_header->old_licensee_code);
  printf("Mask Rom Version Number: %.2X\n",
         cartridge_header->mask_rom_version_number);
  printf("Header Checksum: %.2X\n", cartridge_header->header_checksum);
  printf("Global Checksum: %.2X%.2X\n", cartridge_header->global_checksum_hi,
         cartridge_header->global_checksum_lo);
}

bool cart_load(const char *filename) {
  std::streampos size;
  std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
  if (file.is_open()) {
    size = file.tellg();
    file.seekg(0, std::ios::beg);
    file.read((char *)cartridge_data, MAX_CART_SIZE);
    file.close();
    printf("ROM %s loaded, size %lli bytes\n", filename, std::streamoff(size));
    return true;
  }
  printf("Failed to load file %s\n", filename);
  return false;
}
