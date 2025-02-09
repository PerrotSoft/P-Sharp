#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>

using namespace std;

unordered_map<string, string> variables;

vector<string> split(const string& str) {
    vector<string> tokens;
    string token;
    for (char c : str) {
        if (c == ' ') {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        }
        else {
            token += c;
        }
    }
    if (!token.empty()) tokens.push_back(token);
    return tokens;
}

string compile_to_nasm(const vector<string>& lines) {
    string nasm_code = "";
    string end_code="";
    string data_code = "";
    string bssdata_code = "";
    int g = 0;
    for (const string& line : lines) {
        vector<string> parts = split(line);
        if (parts.empty()) continue;

        string cmd = parts[0];

        if (cmd == "goto") {
            nasm_code += "jmp " + parts[1] + "\n";
        }
        else if(cmd == "consolemode")
        {
            bssdata_code += "buffer resb 256\n";
            end_code += "print:\nadd si,512\nmov bx, si; Prints a string from SI in real mode\nmov ah, 0x0e\nl:\nlodsb\ntest al, al\njz q\nint 10h\njmp l\nq :\nret\n";
            end_code += R"(read:
                mov ah, 0x00; Ожидание ввода символа
                int 0x16; Вызов BIOS(чтение символа)

                cmp al, 0x0D; Если Enter(код 0x0D), конец ввода
                je done

                mov ah, 0x0E; BIOS - функция вывода символа
                int 0x10; Выводим символ

                mov[si], al; Сохраняем символ в буфер
                inc si; Сдвигаем указатель
                jmp read; Повторяем цикл

                done :
            mov byte[si], 0; Завершаем строку нулевым символом

                ; Выводим введённую строку
                mov si, buffer
                print_loop :
            lodsb; Загружаем символ в AL
                test al, al; Проверяем на конец строки(нулевой байт)
                jz halt; Если 0, завершаем

                mov ah, 0x0E; BIOS - функция вывода
                int 0x10; Печатаем символ
                jmp print_loop
                halt :
            ret)"; end_code += "\n";
        }
        else if (cmd == "asm")
        {
            string text = "";
            // Assuming 'parts' is an array or a vector and 'Leang' is the correct property you meant to use
            for (int i = 1; i != parts.size(); i++) {
                text += parts[i] + " ";
            }
            nasm_code += text + "\n";
        }
        else if (cmd == "org")
        {
            nasm_code += "org " + parts[1] + "\n";
        }
        else if (cmd == "savemod") {
            nasm_code += "cli\n";
        }
        else if (cmd == "loadbytes") {
            nasm_code += "mov ah, 0x02\n";
            nasm_code += "mov al, " + parts[1] + "\n";
            nasm_code += "mov bx, " + parts[2] + "\n";
            nasm_code += "int 0x13\n";
        }
        else if (cmd == "savebytes") {
            nasm_code += "mov ah, 0x03\n";
            nasm_code += "mov al, " + parts[1] + "\n";
            nasm_code += "mov bx, " + parts[2] + "\n";
            nasm_code += "int 0x13\n";
        }
        else if (cmd == "print") {
            string text = "";
            for (int i = 1; i != parts.size(); i++) {
                text += parts[i] + " ";
            }
            nasm_code += "mov si, " + text + "\n";
            nasm_code += "call print\n";
        }
        else if (cmd == "read") {
            nasm_code += "mov si, buffer  ; Указатель на начало буфера\n";
            nasm_code += "call read\n";
            nasm_code += "mov si, buffer\n";
            nasm_code += "mov di, " + parts[1] + "\n";
            nasm_code += "copy_loop:\n";
            nasm_code += "lodsb          ; Загружаем байт из [SI] в AL и увеличиваем SI\n";
            nasm_code += "stosb          ; Записываем байт из AL в [DI] и увеличиваем DI\n";
            nasm_code += "test al, al    ; Проверяем конец строки (нулевой символ)\n";
            nasm_code += "jnz copy_loop  ; Если не конец, продолжаем копирование\n";
        }

        else if (cmd == "varbss") {
            bssdata_code += parts[1] + " resb " + parts[2] + "\n";
        }
        else if (cmd == "var") {
            data_code += parts[1] + ": db " + parts[2] + "\n";
        }
        else if (cmd == "valve")
        {
            nasm_code += "#define " + parts[1] + " " + parts[2] + "\n";
        }
        else if (cmd == "if") {
            nasm_code += "cmp " + parts[1] + ", " + parts[2] + "\n";
            nasm_code += "jne " + parts[3] + "\n";
        }
        else if (cmd == "while") {
            nasm_code += "cmp " + parts[1] + ", " + parts[2] + "\n";
            nasm_code += "jae " + parts[3] + "\n";
        }
        else if (cmd == "function") {
            nasm_code += parts[1] + ":\n";
        }
        else if (cmd == "call") {
            nasm_code += "call " + parts[1] + "\n";
        }
        else if (cmd == "add") {
            nasm_code += "add " + parts[1] + ", " + parts[2] + "\n";
        }
        else if (cmd == "sub") {
            nasm_code += "sub " + parts[1] + ", " + parts[2] + "\n";
        }
        else if (cmd == "mul") {
            nasm_code += "mul " + parts[1] + "\n";
        }
        else if (cmd == "div") {
            nasm_code += "div " + parts[1] + "\n";
        }
        else if (cmd == "mod") {
            nasm_code += "mov dx, 0\n";
            nasm_code += "div " + parts[1] + "\n";
            nasm_code += "mov " + parts[1] + ", dx\n";
        }
    }
    nasm_code += end_code + "section .bss\n" + bssdata_code + "section .data\n" + data_code;
    return nasm_code;
}

int main(int argc, char* argv[]) {
    string nasm_output;
    string out = "output.asm";

    if (argc > 1) {
        ifstream file(argv[1]);
        if (!file) {
            cerr << "Не удалось открыть файл: " << argv[1] << endl;
            return 1;
        }
        vector<string> source_code;
        string line;
        while (getline(file, line)) {
            source_code.push_back(line);
        }
        file.close();

        nasm_output = compile_to_nasm(source_code);

        ofstream out_file(out);
        if (!out_file) {
            cerr << "Failed to create file: " << out << endl;
            return 1;
        }
        out_file << nasm_output;
        out_file.close();

        cout << "Compilation complete! File " << out << " создан." << endl;
    }
    else {
        cerr << "No command line arguments provided!" << endl;
        return 1;
    }

    return 0;
}

