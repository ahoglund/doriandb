require_relative "spec_helper"

describe "database" do
  DB_PROMPT = "db>"
  def run_script(commands)
    raw_output = nil
    `make`
    IO.popen("./db", "r+") do |pipe|
      commands.each { |command| pipe.puts command }

      raw_output = pipe.gets(nil)

      pipe.close
    end
    raw_output.split("\n")
  end

  it "inserts and selects a row" do
    commands = [
      "insert 1 test_user test_user@github.com",
      "select",
      ".exit",
    ]

    expected_output = [
      "#{DB_PROMPT} Executed.",
      "#{DB_PROMPT} 1, test_user, test_user@github.com",
      "Executed.",
      "#{DB_PROMPT} Bye!",
    ]

    output = run_script(commands)

    expect(output).to eq expected_output
  end

  it 'prints error message when table is full' do
    script = (1..1401).map do |i|
      "insert #{i} user#{i} person#{i}@example.com"
    end
    script << ".exit"
    result = run_script(script)
    expect(result[-2]).to eq("#{DB_PROMPT} Error: Table full.")
  end

  it "allows strings that are at the max length" do
    name = "a" * 32
    email = "a" * 255

    input = [
      "insert 1 #{name} #{email}",
      "select",
      ".exit",
    ]

    expected_output = [
      "#{DB_PROMPT} Executed.",
      "#{DB_PROMPT} 1, #{name}, #{email}",
      "Executed.",
      "#{DB_PROMPT} Bye!",
    ]

    output = run_script(input)

    (0..(input.length - 1)).each do |i|
      expect(output[i]).to eq expected_output[i]
    end
  end

  it "prints error when strings are too long" do
    name = "a" * 33
    email = "a" * 256

    input = [
      "insert 1 #{name} #{email}",
      "select",
      ".exit",
    ]

    expected_output = [
      "#{DB_PROMPT} String is too long.",
      "#{DB_PROMPT} Executed.",
      "#{DB_PROMPT} Bye!",
    ]

    output = run_script(input)

    (0..(input.length - 1)).each do |i|
      expect(output[i]).to eq expected_output[i]
    end
  end
end
