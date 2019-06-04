#include <piccante.hpp>

void checkWritingValid(bool flag)
{
	if (flag) {
		printf(" -- Result: Ok\n");
	}
	else {
		printf(" -- Result: Writing had some issues!\n");
	}
}


int main(int argc, char *argv[])
{
    std::string img_str;

    if(argc == 2) {
        img_str = argv[1];
    } else {
        img_str = "../resources/input/1.hdr";
    }

    printf("Reading an HDR file...");

    pic::Image img;
    img.Read(img_str);

    printf("Ok\n");

    printf("Is it valid? ");
    if(img.isValid()) {
        printf("OK\n");

        std::string nameOut = pic::getFileNameOnly(img_str);

        bool bWritten;
        pic::Image *image_tmo = NULL;

        printf("Tone mapping mode: Schlick TMO");
        image_tmo = pic::SchlickTMO::execute(&img, image_tmo);

        bWritten = image_tmo->Write("../resources/output/" + nameOut + "_schlick_tmo.png", pic::LT_NOR_GAMMA);

		checkWritingValid(bWritten);

        printf("Tone mapping mode: Global Reinhard TMO");
        image_tmo = pic::ReinhardTMO::executeGlobal1(&img, image_tmo);

        bWritten = image_tmo->Write("../resources/output/" + nameOut + "_reinhard_gtmo.png", pic::LT_NOR_GAMMA);

		checkWritingValid(bWritten);

        printf("Tone mapping mode: Local Reinhard TMO");
        image_tmo = pic::ReinhardTMO::executeLocal1(&img, image_tmo);

        bWritten = image_tmo->Write("../resources/output/" + nameOut + "_reinhard_ltmo.png", pic::LT_NOR_GAMMA);

		checkWritingValid(bWritten);

        printf("Tone mapping mode: Drago TMO");
        image_tmo = pic::DragoTMO::execute(&img, image_tmo);

        bWritten = image_tmo->Write("../resources/output/" + nameOut + "_drago_tmo.png", pic::LT_NOR_GAMMA);

		checkWritingValid(bWritten);

        printf("Tone mapping mode: Ward Histogram Adjustment TMO");
        image_tmo = pic::WardHistogramTMO::execute(&img, image_tmo);

        bWritten = image_tmo->Write("../resources/output/" + nameOut + "_ward_tmo.png", pic::LT_NOR_GAMMA);

		checkWritingValid(bWritten);

    } else {
        printf("No, the file is not valid!\n");
    }

    return 0;
}