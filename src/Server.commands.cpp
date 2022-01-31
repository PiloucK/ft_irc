
#include "Server.hpp"


/**
 * @brief Command NICK from IRC Protocol 
 * 
 * 
 * We have 4 cases 
 * 1. If User want changes his nickname to an another (same FD)
 * 2. If the User have the same nickname to another, kick the two user 
 * 3. If it's a new user, we put the nickname in unnamed_users map. 
 * 4. If the nickname is not in the block_list
 * the void* is cast into a fd (int)
 * 
 * @param void* sender 
 *@sa RFC 2812 (3.1.2)
 */
void irc::Server::cmd_nick(void *sender)
{
    const int fd = *(reinterpret_cast<int*>(sender)); 
    
    std::string nick; 
    std::string tmp(_main_buffer);
    std::size_t found = tmp.find("NICK") + 4; 

    tmp.copy((char*)nick.c_str(), tmp.size() - found , found);
    std::map<std::string, irc::User>::iterator connected_it = _connected_users.find(nick); 
    std::map<int, std::string>::iterator unnamed_it;
    
    // need to check if nickname is ban 

    if (connected_it != _connected_users.end())
    {
        if(connected_it->second._own_socket == fd) 
        {
            User* tmp = connected_it->second; 
            _connected_users.erase(connected_it->second._nickname); 
            tmp->_nickname = nick; 
            _connected_users.insert(std::make_pair(nick, tmp));
            // REPLY TO CLIENT 
        }
        else 
        {
            // SENT ERR MSG IF qui dit que le nickname est deja pris
        }
    }
    else if ((unnamed_it = _unnamed_users.find(fd)) == _unnamed_users.end())
    {
        unnamed_it->second = nick; 
    }
}


/**
 * @brief command USER from IRC Protocl
 * 
 * We have 2 cases : 
 * 1. If the USER command doesn't have username on his request  (461)
 * 2. User is already register (462)
 * 
 * @param input_fd (int)
 * @sa RFC 2812 (3.1.3)
 * 
 */
void irc::Server::cmd_user(void *input_fd)
{
    const int fd = *(reinterpret_cast<int*>(input_fd)); 
    std::string username; 
    std::string tmp(_main_buffer);
    std::size_t start = tmp.find("USER") + 5; 
    std::size_t end = tmp.find(' ', start); 
    std::size_t nb_of_space = std::count(tmp.begin(), tmp.end(), ' ');

    if(nb_of_space == 4)
    {
        send(fd, 461, sizeof(int), MSG_DONTWAIT);
        return ; 
    }

    tmp.copy((char*)username.c_str(), start , end - start);
    std::map<int, std::string>::iterator unnamed_it; 

    if(unnamed_it != _unnamed_users.end())
    {
        std::map<int, std::string>::iterator check_even_connected_it = _connected_users.find(username)->first; 
        
        if(check_even_conneceted_it != _connected_users.end())
        {
            send(fd, 462, sizeof(int), MSG_DONTWAIT); 
            return ;
        }
        
        else if(connected_it->second.size() != 0)
        {
            _connected_users.insert(User(_connected_it->second, username, fd));
            _unnamed_users.erase(fd); 
        }
    }
}

/**
 * @brief Hash Password 
 * 
 * This method take a string as parameter and transform inch character as int value transform by arthimetical operations.
 * the method return an array of int with encrypted password. 
 * 
 * @param input_pass (char*) 
 * @return int* 
 */
int*    irc::Server::pass_hash(char *input_pass)
{
    int len = strlen(input_pass); 
    int* ret = new int[len]; 
    for (int i = 0; i < len; i++)
    {
        ret[i] = (input_pass[i] * len) - (input_pass[i] - i); 
    }
    return (ret); 
}

/**
 * @brief Command PASS from IRC Protocol 
 * 
 * We have 3 cases : 
 * 1. If the users is already register (in unnamed users map, find by fd)
 * 2. If we have no argument (no argument after "PASS" command)
 * 3. If the pass given by the user doesn't match with the server password.
 * 
 * @param input_fd (int)
 * @sa RFC 2812 (3.1.1)
 * 
 */
void irc::Server::cmd_pass(void *input_fd)
{
    const int fd = *(reinterpret_cast<int*>(input_fd));
    std::map<int, std::string>::iterator unnamed_it = _unnamed_users.find(fd)->first; 
    
    if(unnamed_it != _unnamed_users.end())
    {
        send(fd, 462, sizeof(int), MSG_DONTWAIT); 
        return ;
    }

    std::string input_pass(_main_buffer);
    std::string raw_pass; 
    
    if(input_pass.size() < 6)
    {
        send(fd, 461, sizeof(int), MSG_DONTWAIT); 
        return ;
    }

    input_pass.copy((char*)raw_pass.c_str(), input_pass.size() - 6 , 6);
    int *hash_pass = pass_hash((char*)raw_pass.c_str()); 
    
    for(int i = 0; i < strlen(raw_pass.c_str()); i++)
    {
        if(hash_pass[i] != _server_pass[i])
        {
            delete(hash_pass);
            return;
        }
    }

    _unnamed_users.insert(make_pair(fd, "")); 
    delete(hash_pass); 
}

