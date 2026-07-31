print("Hello world")
aimport adsk.core, adsk.fusion, traceback
import shutil, zipfile, urllib.request, os
from html.parser import HTMLParser

def download_from_gobilda(urlpath):



    class MyHTMLParser(HTMLParser):
        def __init__(self):
            super().__init__()
            self.curr_tag = None
            self.title = None
            self.sku = None

        def handle_starttag(self, tag, attrs):
            self.curr_tag = tag
            if tag == 'div' and attrs:
                if attrs[0][0] == 'data-sku' and not self.sku:
                    self.sku = attrs[0][1]

        def handle_data(self, data):
            if not self.title and self.curr_tag == 'title':
                self.title = data.replace(' - goBILDA', '').replace(":", "-").replace("/", "%").replace("*", "%").replace('"', '%').replace('\\', '%').replace('<', '%').replace('>', '%').replace('|', '%').replace('?', '%')

    print('URL: %s' % urlpath)
    page = urllib.request.urlopen(urlpath)
    page_data = page.read().decode()

    parser = MyHTMLParser()
    parser.feed(page_data)

    print('TITLE: %s' % parser.title)
    print('SKU: %s' % parser.sku)

    sku_zip = '%s.zip' % parser.sku
    sku_url = 'https://www.gobilda.com/content/step_files/%s' % sku_zip
    urllib.request.urlretrieve(sku_url, sku_zip)

    with zipfile.ZipFile(sku_zip, "r") as zip_ref:
        zip_ref.extractall()
        namelist = zip_ref.namelist()

    print(namelist[0])
    step_fname = namelist[0]

    new_fname = '%s.STEP' % (parser.title)
    shutil.copyfile(step_fname, new_fname)
    os.remove("%s.zip" % (parser.sku))
    os.remove(step_fname)

    return new_fname  # Return the new filename for the downloaded STEP file

def upload_to_fusion(filename):
    try:
        app = adsk.core.Application.get()
        ui  = app.userInterface
        design = app.activeProduct
        rootComp = design.rootComponent

        componentName = os.path.splitext(os.path.basename(filename))[0]

        importOptions = app.importManager.createSTEPImportOptions(filename)
        app.importManager.importToTarget(importOptions, rootComp)

        lastOcc = rootComp.occurrences[-1]
        lastOcc.component.name = componentName

        ui.messageBox(f'File {componentName} imported successfully.')

    except:
        if ui:
            ui.messageBox('Failed:\n{}'.format(traceback.format_exc()))

def run(context):
    app = adsk.core.Application.get()
    ui = app.userInterface

    urlpath, dialogResult = ui.inputBox('Please enter goBILDA URL:', 'goBILDA URL', '')

    if dialogResult == adsk.core.DialogResults.DialogOK and urlpath:
        downloaded_file = download_from_gobilda(urlpath)
        upload_to_fusion(downloaded_file)
    else:
        ui.messageBox('No URL entered or operation cancelled.')
