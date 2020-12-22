#ifndef DARKSTARDTSCONVERTER_BITMAP_HPP
#define DARKSTARDTSCONVERTER_BITMAP_HPP

#include <vector>
#include <array>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <limits>
#include "palette.hpp"
#include "endian_arithmetic.hpp"

namespace darkstar::bmp
{
  namespace endian = boost::endian;
  using file_tag = std::array<std::byte, 4>;

  constexpr file_tag to_tag(const std::array<std::uint8_t, 4> values)
  {
    file_tag result{};

    for (auto i = 0u; i < values.size(); i++)
    {
      result[i] = std::byte{ values[i] };
    }
    return result;
  }

  constexpr file_tag pbmp_tag = to_tag({ 'P', 'B', 'M', 'P' });

  constexpr file_tag header_tag = to_tag({ 'h', 'e', 'a', 'd' });

  constexpr file_tag data_tag = to_tag({ 'd', 'a', 't', 'a' });

  constexpr file_tag detail_tag = to_tag({ 'D', 'E', 'T', 'L' });

  constexpr file_tag palette_tag = to_tag({ 'P', 'i', 'D', 'X' });

  constexpr std::array<std::byte, 2> windows_bmp_tag = { std::byte{66}, std::byte{77}}; // BM

  constexpr std::array<std::byte, 2> special_reserved_tag = { std::byte{0xF7}, std::byte{0xF5}}; // for palettes

  struct pbmp_header
  {
    endian::little_uint32_t version;
    endian::little_int32_t width;
    endian::little_int32_t height;
    endian::little_uint32_t bit_depth;
    endian::little_uint32_t flags;
  };

  struct windows_bmp_header
  {
    std::array<std::byte, 2> tag;
    endian::little_uint32_t file_size;
    endian::little_uint16_t reserved1;
    endian::little_uint16_t reserved2;
    endian::little_uint32_t offset;
  };

  struct windows_bmp_info
  {
    endian::little_uint32_t info_size;
    endian::little_int32_t width;
    endian::little_int32_t height;
    endian::little_uint16_t planes;
    endian::little_uint16_t bit_depth;
    endian::little_uint32_t compression;
    endian::little_uint32_t image_size;
    endian::little_int32_t x_pixels_per_metre;
    endian::little_int32_t y_pixels_per_metre;
    endian::little_uint32_t num_colours_used;
    endian::little_uint32_t num_important_colours;
  };



  void get_bmp_data(std::basic_ifstream<std::byte>& raw_data)
  {
    windows_bmp_header header{};
    raw_data.read(reinterpret_cast<std::byte*>(&header), sizeof(header));

    if (header.tag != windows_bmp_tag)
    {
      throw std::invalid_argument("File data is not BMP based.");
    }

    windows_bmp_info info{};

    raw_data.read(reinterpret_cast<std::byte*>(&info), sizeof(info));

    int num_colours = 0;

    if (info.bit_depth == 4)
    {
      num_colours = 16;
    }
    else if (info.bit_depth == 8)
    {
      num_colours = 256;
    }

    std::vector<pal::colour> colours;
    colours.reserve(num_colours);

    for (auto i = 0; i < num_colours; ++i)
    {
      pal::colour colour{};
      raw_data.read(reinterpret_cast<std::byte*>(&colour), sizeof(colour));
      colours.emplace_back(colour);
    }

    const auto num_pixels = info.width * info.height * (info.bit_depth / 8);

    std::vector<std::byte> pixels(num_pixels, std::byte{});

    raw_data.read(pixels.data(), pixels.size());

    for (auto& colour : colours)
    {
      std::cout << "#"
                << std::setfill('0')
                << std::setw(2)
                << std::hex
                << int(colour.red)
                << std::setw(2)
                << int(colour.green)
                << std::setw(2)
                << int(colour.blue)
                << '\n';
    }
  }

  void get_pbmp_data(std::basic_ifstream<std::byte>& raw_data)
  {
    std::array<std::byte, 4> header{};
    endian::little_uint32_t file_size{};

    raw_data.read(header.data(), sizeof(header));
    raw_data.read(reinterpret_cast<std::byte*>(&file_size), sizeof(file_size));

    if (header != pbmp_tag)
    {
      throw std::invalid_argument("File data is not PBMP based.");
    }

    pbmp_header bmp_header{};
    endian::little_uint32_t detail_levels{};
    endian::little_uint32_t palette_index{};
    std::vector<std::byte> pixels;

    while (!raw_data.eof())
    {
      std::array<std::byte, 4> chunk_header{};
      endian::little_uint32_t chunk_size{};

      raw_data.read(chunk_header.data(), chunk_header.size());
      raw_data.read(reinterpret_cast<std::byte*>(&chunk_size), sizeof(chunk_size));

      if (chunk_header == header_tag)
      {
        raw_data.read(reinterpret_cast<std::byte*>(&bmp_header), sizeof(bmp_header));

        const auto num_pixels = bmp_header.width * bmp_header.height * (bmp_header.bit_depth / 8);
        pixels = std::vector<std::byte>(num_pixels, std::byte{});
      }
      else if (chunk_header == data_tag)
      {
        raw_data.read(pixels.data(), pixels.size());

        // PBMP files contain mip maps of the main image.
        // For now, we won't worry about them.
        raw_data.seekg(chunk_size - pixels.size(), std::ios::cur);
      }
      else if (chunk_header == detail_tag)
      {
        raw_data.read(reinterpret_cast<std::byte*>(&detail_levels), sizeof(detail_levels));
      }
      else if (chunk_header == palette_tag)
      {
        raw_data.read(reinterpret_cast<std::byte*>(&palette_index), sizeof(palette_index));
      }
      else
      {
        if (chunk_size == 0)
        {
          break;
        }
        raw_data.seekg(chunk_size, std::ios::cur);
      }
    }
  }
}

#endif//DARKSTARDTSCONVERTER_BITMAP_HPP
