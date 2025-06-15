# Ship management system

Сервис, который представляет собой систему создания и управления своим виртуальным флотом.

### Vulnerability chain
1. Первым делом стоит обратить внимание на файл `СMakeLists.txt` и `captain_handler.cpp`. В этих файлах можно увидеть что основой дальнейшей эксплуатации будет обычное переполнение буффера.

```C++
   crow::response changeSailorDescription(const crow::request &req) {
    ...

    auto body = crow::json::load(req.body);
    std::string encodedDesc = body["new_description"].s();
    std::string decodedDesc = base64_decode(encodedDesc);

    auto decoded = jwt::decode(token);
    std::string firstname = decoded.get_payload_claim("firstname").as_string();
    std::string secondname =
        decoded.get_payload_claim("secondname").as_string();
    std::string thirdname = decoded.get_payload_claim("thirdname").as_string();
    std::string role = decoded.get_payload_claim("role").as_string();

    std::memcpy(new_description, decodedDesc.c_str(), decodedDesc.size()); //уязвимость тут
    new_description[199] = '\0';

    int sailor_id = body["sailor_id"].i();

    ...
   }

```
2. Дальше возможны 2 пути: простой и сложный.  
    Простой: понять что `std::string *error_message` из `changeSailorDescription` это *указатель* на строку. Есть ли еще какие то полезные указатели? ЕСТЬ!!! Обратим внимание на файл `global.cpp` в нем содержится `std::string* globalString`. Что же это такое? А это указатель на jwt secret key. Если мы его получим то сможем подписать любой запрос от имени любого адмирала и используя функционал приложения можно получать все необходимые флаги. 

    Сложный: тк отключены защиты можно попробовать реализовать rop или ret2libc, но я не заморачивался этим.

3. Но как нам получить утечку адрессов. И снова немного поискав можно найти что много ошибок обрабатывается неким `server_error_handler ()`.
Посмотрим что это за функция:
```C++
std::string server_error_handler(const std::string& input){
    std::ostringstream oss;
    oss << input << "\nStack trace:\n"
        << boost::stacktrace::stacktrace(); // тут можно получить лик аддресов
    return oss.str();
}
```
4. Дальше дело техники: считаем смещение, ликаем адресса, ликаем jwt, и читаем все флаги.
