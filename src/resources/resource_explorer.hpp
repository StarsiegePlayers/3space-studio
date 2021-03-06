#ifndef DARKSTARDTSCONVERTER_RESOURCE_EXPLORER_HPP
#define DARKSTARDTSCONVERTER_RESOURCE_EXPLORER_HPP

#include <filesystem>
#include <memory>
#include <map>
#include <algorithm>
#include <locale>
#include <fstream>
#include <optional>
#include <functional>
#include <nonstd/span.hpp>
#include "archive_plugin.hpp"

namespace studio::resources
{
  using file_stream = std::pair<studio::resources::file_info, std::unique_ptr<std::basic_istream<std::byte>>>;

  struct null_buffer : public std::basic_streambuf<std::byte>
  {
    int overflow(int c) { return c; }
  };

  class resource_explorer
  {
  public:
    explicit resource_explorer(const std::filesystem::path& search_path) : search_path(search_path) {}

    static std::filesystem::path get_archive_path(const std::filesystem::path& folder_path);
    static void merge_results(std::vector<studio::resources::file_info>& group1,
                              const std::vector<studio::resources::file_info>& group2);

    void add_action(std::string name, std::function<void(const studio::resources::file_info&)> action);

    void execute_action(const std::string& name, const studio::resources::file_info& info) const;

    std::filesystem::path get_search_path() const;

    void add_archive_type(std::string extension, std::unique_ptr<studio::resources::archive_plugin> archive_type, std::optional<nonstd::span<std::string_view>> explicit_extensions = std::nullopt);

    std::vector<studio::resources::file_info> find_files(const std::filesystem::path& new_search_path, const std::vector<std::string_view>& extensions) const;

    std::vector<studio::resources::file_info> find_files(const std::vector<std::string_view>& extensions) const;

    file_stream load_file(const std::filesystem::path& path) const;

    file_stream load_file(const studio::resources::file_info& info) const;

    bool is_regular_file(const std::filesystem::path& folder_path) const;

    std::optional<std::reference_wrapper<studio::resources::archive_plugin>> get_archive_type(const std::filesystem::path& file_path) const;
    void extract_file_contents(std::basic_istream<std::byte>& archive_file, std::filesystem::path destination, const studio::resources::file_info& info) const;
    std::vector<std::variant<studio::resources::folder_info, studio::resources::file_info>> get_content_listing(const std::filesystem::path& folder_path) const;

  private:
    const std::filesystem::path& search_path;

    std::locale default_locale;

    std::map<std::string, nonstd::span<std::string_view>> archive_explicit_extensions;

    std::multimap<std::string, std::unique_ptr<studio::resources::archive_plugin>> archive_types;
    std::map<std::string, std::function<void(const studio::resources::file_info&)>> actions;

    mutable std::map<std::string, std::vector<studio::resources::file_info>> info_cache;
  };
}// namespace studio::resource


#endif//DARKSTARDTSCONVERTER_RESOURCE_EXPLORER_HPP
