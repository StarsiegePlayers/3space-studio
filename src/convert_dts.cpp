#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <filesystem>
#include <boost/endian/arithmetic.hpp>
#include "structures.hpp"

namespace fs = std::filesystem;
namespace dts = darkstar::dts;

std::vector<std::byte> read_string(std::vector<std::byte>::iterator& iterator, std::size_t size)
{
    if (size < 16)
    {
        size++;
    }

    std::vector<std::byte> dest(size);
    std::copy(iterator, iterator + size, dest.begin());
    std::advance(iterator, size);

    return dest;
}


template <typename destination_type>
std::vector<destination_type> read_vector(std::vector<std::byte>::iterator& iterator, std::size_t size)
{
    if (size == 0)
    {
        return {};
    }

    std::vector<destination_type> dest(size);
    std::copy(iterator, iterator + sizeof(destination_type) * size, reinterpret_cast<std::byte*>(&dest[0]));
    std::advance(iterator, sizeof(destination_type) * size);

    return dest;
}


template<std::size_t size>
std::array<std::byte, size> read(std::vector<std::byte>::iterator& iterator)
{
    std::array<std::byte, size> dest{};
    std::copy(iterator, iterator + sizeof(dest), dest.begin());
    std::advance(iterator, sizeof(dest));

    return dest;
}

template <typename destination_type>
destination_type read(std::vector<std::byte>::iterator& iterator)
{
    destination_type dest{};
    std::copy(iterator, iterator + sizeof(destination_type), reinterpret_cast<std::byte*>(&dest));
    std::advance(iterator, sizeof(destination_type));

    return dest;
}

dts::tag_header read_file_header(std::vector<std::byte>::iterator& cursor)
{
    dts::tag_header file_header =
            {
                    read<sizeof(dts::file_tag)>(cursor),
                    read<dts::file_info>(cursor)
            };

    if (file_header.tag != dts::pers_tag)
    {
        throw std::invalid_argument("The file provided does not have the appropriate tag to be a Darkstar DTS file.");
    }

    file_header.class_name = read_string(cursor, file_header.file_info.class_name_length);
    file_header.version = read<dts::version>(cursor);

    return file_header;
}

int main(int argc, const char** argv)
{
    if (argc > 1)
    {
        std::string_view file_name = argv[1];
        auto file_size = fs::file_size(file_name);
        std::vector<std::byte> file_buffer(file_size);
        std::basic_ifstream<std::byte> input(file_name, std::ios::binary);

        input.read(&file_buffer[0], file_size);

        auto cursor = file_buffer.begin();

        dts::tag_header file_header = read_file_header(cursor);

        auto header = read<dts::shape::v7::header>(cursor);
        dts::shape_v7 shape
        {
            header,
            read<dts::shape::v7::data>(cursor),
            read_vector<dts::shape::v7::node>(cursor, header.num_nodes),
            read_vector<dts::shape::v7::sequence>(cursor, header.num_sequences),
            read_vector<dts::shape::v7::sub_sequence>(cursor, header.num_sub_sequences),
            read_vector<dts::shape::v7::keyframe>(cursor, header.num_key_frames),
            read_vector<dts::shape::v7::transform>(cursor, header.num_transforms),
            read_vector<dts::shape::v7::name>(cursor, header.num_names),
            read_vector<dts::shape::v7::object>(cursor, header.num_objects),
            read_vector<dts::shape::v7::detail>(cursor, header.num_details),
            read_vector<dts::shape::v7::transition>(cursor, header.num_transitions),
            read_vector<dts::shape::v7::frame_trigger>(cursor, header.num_frame_triggers),
            read<dts::shape::v7::footer>(cursor)
        };

        shape.meshes.reserve(header.num_meshes);

        for (int i = 0; i < header.num_meshes; ++i)
        {
            auto mesh_tag_header = read_file_header(cursor);
            auto mesh_header = read<dts::mesh::v3::header>(cursor);

            std::cerr << "num_verts " << mesh_header.num_verts << std::endl;
            std::cerr << "num_texture_verts " << mesh_header.num_texture_verts << std::endl;
            std::cerr << "num_faces " << mesh_header.num_faces << std::endl;

            assert(mesh_header.num_verts < 65000);
            assert(mesh_header.num_texture_verts < 65000);

            std::cerr << (char*)&mesh_tag_header.class_name[0] << std::endl;
            dts::mesh_v3 mesh {
                    mesh_header,
                    read_vector<dts::mesh::v3::vertex>(cursor, mesh_header.num_verts),
                    read_vector<dts::mesh::v3::texture_vertex>(cursor, mesh_header.num_texture_verts),
                    read_vector<dts::mesh::v3::face>(cursor, mesh_header.num_faces),
                    read_vector<dts::mesh::v3::frame>(cursor, mesh_header.num_frames)
            };

            shape.meshes.push_back(mesh);
        }

        // TODO fix material list parsing and get that working
        if (auto has_material_list = read<dts::shape::v7::has_material_list_flag>(cursor); has_material_list == 1)
        {
            auto material_list_header = read_file_header(cursor);

            std::cout << (char*)&material_list_header.class_name[0] << std::endl;
            std::cout << material_list_header.version << std::endl;
        }

        std::cout << shape.footer.always_node << " " << shape.footer.num_default_materials << '\n';

        std::cout << shape.transforms.back().scale.x << " " << shape.transforms[0].scale.y << '\n';

        std::cout << (char*)&shape.names.back() << '\n';

        for (auto& byte : file_header.tag)
        {
            std::cout << (char)byte << '\n';
        }

        std::cout << file_header.version << '\n';
        std::cout << (char*)&file_header.class_name[0] << '\n';

        std::cout << shape.header.num_nodes << '\n';

        std::cout << shape.nodes[0].num_sub_sequences << '\n';

        std::cout << shape.sequences[0].name_index << '\n';

        std::cout << shape.data.radius << '\n';

        std::cout << "Main file is " << file_header.file_info.file_length << " bytes in size\n";
        std::cout << "Class name length is " << file_header.file_info.class_name_length << " bytes in size\n";

        std::cout << "The file size of " << argv[0] << " is: " << file_size << '\n';
    }

    return 0;
}