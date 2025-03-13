import subprocess
import argparse
import os


def convert_asc_to_g(input_file):
    try:
        output_file = os.path.splitext(input_file)[0] + '.g'

        result = subprocess.run(['asc2g', input_file, output_file], check=True, capture_output=True, text=True)

        if result.returncode == 0:
            print(f"Conversion successful: {input_file} -> {output_file}")
        else:
            print(f"Error in conversion: {result.stderr}")

    except subprocess.CalledProcessError as e:
        print(f"Command failed with error: {e}")


def convert_all_asc_in_directory(directory):
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith('.asc'):
                input_file = os.path.join(root, file)
                print(f"Processing {input_file}...")
                convert_asc_to_g(input_file)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Convert ASC files to G format.")
    parser.add_argument(
        '--single',
        type=str,
        help='Convert a single ASC file to G format. Provide input path',
        nargs=1
    )
    parser.add_argument(
        '--directory',
        type=str,
        help='Convert all ASC files in a directory to G format.'
    )

    args = parser.parse_args()

    if args.single:
        input_file = args.single[0]
        convert_asc_to_g(input_file)

    elif args.directory:
        convert_all_asc_in_directory(args.directory)
    else:
        print("Please provide either --single or --directory options.")