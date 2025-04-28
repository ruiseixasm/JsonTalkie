def main():
    print("Welcome to My Command Line!")
    print("Type 'help' to see available commands.")
    
    while True:
        command = input("> ").strip()
        
        if command == "help":
            print("Available commands: greet, bye, exit")
        
        elif command == "greet":
            print("Hello, world!")
        
        elif command == "bye":
            print("Goodbye!")
        
        elif command == "exit":
            print("Exiting...")
            break
        
        else:
            print(f"Unknown command: {command}")

if __name__ == "__main__":
    main()
