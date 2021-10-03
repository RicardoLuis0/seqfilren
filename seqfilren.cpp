#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <vector>
#include <array>
#include <string>
#include <iostream>
#include <regex>

namespace std {
    namespace fs=filesystem;
}

namespace {
    
    size_t start_output_index=1;
    std::unordered_map<std::string,std::vector<std::fs::path>> ext_files;
    std::unordered_map<std::string,size_t> ext_indexes;
    
    std::unordered_set<std::string> allowed_exts;
    bool use_allowed_exts=false;
    
    bool same_index=false;
    bool test_only=false;
    bool do_copy=false;
    bool recursive=false;
    bool no_prompt=false;
    bool use_regex=false;
    bool regex_use_path=false;
    bool regex_use_ext=false;
    
    std::string regex_str;
    
    std::regex regex;
    
    std::fs::path out_folder;
    
    std::string prefix;
    
    std::string trim(const std::string &str){
        constexpr std::array<char,4> whitespace {
            ' ',
            '\r',
            '\n',
            '\t',
        };
        auto begin=str.begin();
        auto end=str.end()-1;
        while(std::find(whitespace.begin(),whitespace.end(),*begin)!=whitespace.end())begin++;
        while(std::find(whitespace.begin(),whitespace.end(),*end)!=whitespace.end())end--;
        return std::string(begin,end+1);
    }
    
    void add_allowed_ext(const std::string &ext){
        if(ext.size()>0){
            if(ext[0]!='.'){
                allowed_exts.emplace("."+ext);
            }else{
                allowed_exts.emplace(ext);
            }
            use_allowed_exts=true;
        }
    }
    
    bool ext_is_allowed(const std::string &ext){
        if(use_allowed_exts){
            return allowed_exts.find(ext)!=allowed_exts.end();
        }else{
            return true;
        }
    }
    
    void add_file(const std::fs::path &p){
        std::string ext=p.extension().string();
        if(ext_is_allowed(ext)){
            ext_files[ext].push_back(p);
        }
    }
    
    size_t ext_index(const std::string &ext){
        static size_t global_index=start_output_index;
        if(same_index){
            return global_index++;
        }else{
            if(auto it=ext_indexes.find(ext);it!=ext_indexes.end()){
                return it->second++;
            }else{
                ext_indexes.insert({ext,start_output_index+1});
                return start_output_index;
            }
        }
    }
    
    void execute(const std::fs::path &from,const std::fs::path &to,bool overwrite){
        std::cout<<std::fs::relative(from)<<" -> "<<std::fs::relative(to)<<"\n";
        if(!test_only){
            bool do_overwrite=false;
            if(std::fs::exists(to)){
                if(overwrite){
                    do_overwrite=true;
                }else{
                    std::cerr<<std::fs::relative(to)<<" skipped, target file already exists\n";
                    return;
                }
            }
            if(do_copy){
                std::fs::copy(from,to,do_overwrite?std::fs::copy_options::overwrite_existing:std::fs::copy_options::none);
            }else{
                if(do_overwrite){
                    std::fs::remove(to);
                }
                std::fs::rename(from,to);
            }
        }
    }
    
    void execute_op(const std::tuple<std::fs::path,std::fs::path,bool> &op){
        execute(std::get<0>(op),std::get<1>(op),std::get<2>(op));
    }
    
    bool gen_ops(const std::filesystem::path &root,std::vector<std::tuple<std::fs::path,std::fs::path,bool>> &ops){
        if(std::fs::exists(root)){
            if(std::fs::is_directory(root)){
                for(const auto &e:std::fs::directory_iterator(root)){
                    if(use_regex){
                        std::string search_str=(regex_use_path&&regex_use_ext)?
                                                    std::fs::relative(e).string():
                                                regex_use_path?
                                                    std::fs::relative(e).replace_extension().string():
                                                regex_use_ext?
                                                    std::fs::relative(e).filename().string():
                                                    std::fs::relative(e).replace_extension().filename().string();
                        if(!std::regex_search(search_str,regex)){
                            continue;
                        }
                    }
                    if(e.is_regular_file()){
                        add_file(e.path());
                    }else if(recursive&&e.is_directory()){
                        gen_ops(e.path(),ops);
                    }
                }
            }else if(std::fs::is_regular_file(root)){
                add_file(root);
            }else{
                goto fail;
            }
            for(auto &ext_filv:ext_files){
                ops.reserve(ops.size()+ext_filv.second.size());
                for(auto &fil:ext_filv.second){
                    bool overwrite=false;
                    std::fs::path outfil;
                    while(true){
                        outfil=out_folder/(prefix+std::to_string(ext_index(ext_filv.first))+ext_filv.first);
                        if(!std::fs::exists(outfil)){
                            break;
                        }else{
                            if(no_prompt){
                                continue;
                            }else{
                                while(true){
                                    std::cout<<std::fs::relative(outfil)<<" already exists, (S)kip, "<<(std::fs::is_regular_file(outfil)?"(O)verwrite, ":"")<<"(R)ename or (C)ancel?";
                                    std::string in;
                                    std::getline(std::cin,in);
                                    if(in=="skip"||in=="Skip"||in=="s"||in=="S"){
                                        goto continue_outer_loop;
                                    }else if(in=="overwrite"||in=="Overwrite"||in=="o"||in=="O"){
                                        if(std::fs::is_regular_file(outfil)){
                                            overwrite=true;
                                            goto end_inner_loop;
                                        }else{
                                            std::cout<<std::fs::relative(outfil)<<" is not a file, cannot overwrite\n";
                                        }
                                    }else if(in=="cancel"||in=="Cancel"||in=="c"||in=="C"){
                                        return false;
                                    }else if(in=="rename"||in=="Rename"||in=="r"||in=="R"){
                                        break;
                                    }
                                }
                            }
                        }
                        continue;
                    end_inner_loop:
                        break;
                    }
                    ops.emplace_back(fil,outfil,overwrite);
                }
            continue_outer_loop:;
            }
        }
        return true;
    fail:
        throw std::runtime_error(root.string()+" is not a valid file/folder");
    }
    
    bool starts_with(const std::string &s,const std::string &what){
        return s.rfind(what,0)==0;
    }
    
    std::string get_help_str(const std::string &prog){
        return "Usage:\n"+
               prog+" [options] ...input files/folders\n"
                    "\nif no inputs are specified, will scan current folder\n\n"
                    "Options:\n"
                    " SHORT            LONG                  DESCRIPTION\n"
                    "  -h            --help              display this help message\n"
                    "  -s            --same_index        use the same index for files with different extensions\n"
                    "  -r            --recursive         scan subfolders recursively\n"
                    "  -t            --test              don't perform any operations, only display actions that would have been taken\n"
                    "  -c            --copy              don't rename/move files, copy them instead\n"
                    "  -y            --yes               don't prompt for confirmation\n"
                    "  -p=[prefix]   --prefix=[prefix]   output file prefix\n"
                    "  -o=[folder]   --output=[folder]   output folder\n"
                    "  -e=[ext]      --extension=[ext]   extension to search for, may be defined multiple times, if not defined, will search all files in folder, leading dot not required, but supported\n"
                    "  -m=[rxp]      --match_regex=[rxp] require files to match regular expression\n"
                    "                --match_regex_ext   include extension in regex match string\n"
                    "                --match_regex_path  include relative path to current folder in regex match string\n"
                    "  -z            --zero_index        start file index at zero instead of one, same as -i=0\n"
                    "  -i=[int]      --start_index=[int] specify exact starting file index\n";
    }
}

struct hash_path {
    size_t operator()(const std::fs::path &p) const{
        return std::fs::hash_value(p);
    }
};

int main(int argc,char ** argv) try {
    out_folder=std::fs::current_path();
    std::unordered_set<std::fs::path,hash_path> files;
    if(argc>1){
        std::vector<std::string> args;
        std::unordered_set<std::string> unknown_args;
        std::vector<std::string> errors;
        bool print_help=false;
        args.reserve(argc-1);
        for(int i=1;i<argc;i++){
            args.emplace_back(argv[i]);
        }
        for(const std::string &arg:args){//very rudimentary cmdline arg parsing
            if(starts_with(arg,"-e=")){
                add_allowed_ext(trim(arg.substr(3)));
            }else if(starts_with(arg,"--extension=")){
                add_allowed_ext(trim(arg.substr(12)));
            }else if(arg=="-s"||arg=="--same_index"){
                same_index=true;
            }else if(arg=="-r"||arg=="--recursive"){
                recursive=true;
            }else if(arg=="-h"||arg=="--help"){
                print_help=true;
            }else if(arg=="-t"||arg=="--test"){
                test_only=true;
            }else if(arg=="-c"||arg=="--copy"){
                do_copy=true;
            }else if(arg=="-y"||arg=="--yes"){
                no_prompt=true;
            }else if(starts_with(arg,"-p=")){
                prefix=trim(arg.substr(3));
            }else if(starts_with(arg,"--prefix=")){
                prefix=trim(arg.substr(9));
            }else if(starts_with(arg,"-o=")){
                out_folder=std::fs::current_path()/trim(arg.substr(3));
            }else if(starts_with(arg,"--output=")){
                out_folder=std::fs::current_path()/trim(arg.substr(9));
            }else if(arg=="-z"||arg=="--zero_index"){
                start_output_index=0;
            }else if(starts_with(arg,"-m=")){
                regex_str=trim(arg.substr(3));
                use_regex=true;
            }else if(starts_with(arg,"--match_regex=")){
                regex_str=trim(arg.substr(14));
                use_regex=true;
            }else if(arg=="--match_regex_ext"){
                regex_use_ext=true;
            }else if(arg=="--match_regex_path"){
                regex_use_path=true;
            }else if(starts_with(arg,"-i=")||starts_with(arg,"--start_index=")){
                std::string istr=trim(arg.substr(arg[1]=='-'?14:3));
                try {
                    if constexpr(sizeof(unsigned long)==sizeof(size_t)){
                        start_output_index=std::stoul(istr);
                    }else{
                        start_output_index=std::stoull(istr);
                    }
                }catch(std::invalid_argument &e){
                    errors.push_back("Index number '"+istr+"' is not a valid number");
                }catch(std::out_of_range &e){
                    errors.push_back("Index '"+istr+"' out of range");
                }
            }else if(arg.size()>0&&arg[0]=='-'){
                unknown_args.emplace(arg);
            }else{
                files.emplace(arg);
            }
        }
        if(unknown_args.size()>0||errors.size()>0){
            if(unknown_args.size()>0){
                if(unknown_args.size()==1){
                    std::cerr<<"Unkown Argument: "<<*unknown_args.begin()<<"\n";
                }else{
                    std::cerr<<"Unknown Arguments:\n";
                    bool first=true;
                    for(auto &arg:unknown_args){
                        if(!first){
                            std::cerr<<"\n";
                        }
                        std::cerr<<"  "<<arg;
                        first=false;
                    }
                    std::cerr<<"\n";
                }
            }
            if(errors.size()>0){
                if(errors.size()==1){
                    std::cerr<<"Error: "<<*errors.begin()<<"\n";
                }else{
                    std::cerr<<"Error:\n";
                    bool first=true;
                    for(auto &error:errors){
                        if(!first){
                            std::cerr<<"\n";
                        }
                        std::cerr<<"  "<<error;
                        first=false;
                    }
                    std::cerr<<"\n";
                }
            }
            std::cerr<<"\n"<<get_help_str(argv[0]);
            return 1;
        }else if(print_help){
            std::cout<<"-- Sequential file renamer --\n"<<get_help_str(argv[0]);
            return 0;
        }
    }
    if(use_regex){
        regex=std::regex(regex_str);
    }
    std::vector<std::tuple<std::fs::path,std::fs::path,bool>> ops;
    if(files.size()>0){
        for(auto &f:files){
            if(!gen_ops(f,ops)){
                std::cout<<"Operation Cancelled.\n";
                return 0;
            }
        }
    }else{
        if(!gen_ops(std::fs::current_path(),ops)){
            std::cout<<"Operation Cancelled.\n";
            return 0;
        }
    }
    
    if(!test_only&&!no_prompt&&ops.size()>0){
        std::string msg=std::string("This operation will ")+(do_copy?"create ":(std::fs::current_path()==out_folder?"rename ":"move "))+std::to_string(ops.size())+(ops.size()>1?" files":" file")+". Continue (Yes/No) or (L)ist operations? ";
        while(true){
            std::cout<<msg;
            std::string in;
            std::getline(std::cin,in);
            if(in=="yes"||in=="Yes"||in=="y"||in=="Y"){
                std::cout<<"Continuing...\n";
                break;
            }else if(in=="no"||in=="No"||in=="n"||in=="N"){
                return 1;
            }else if(in=="list"||in=="List"||in=="l"||in=="L"){
                for(auto &op:ops){
                    std::cout<<std::fs::relative(std::get<0>(op))<<" -> "<<std::fs::relative(std::get<1>(op))<<"\n";
                }
            }
        }
    }
    if(!test_only){
        std::fs::create_directories(out_folder);
    }
    for(auto &op:ops){
        execute_op(op);
    }
    return 0;
} catch (std::exception &e){
    std::cerr<<"Unexpected Error: "<<e.what()<<"\n";
    return 1;
}
